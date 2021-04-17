#include "qA.h"
#include "qB.h"
#include "linkedlist.h"
#include "fNode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>

int readArgs(int argc, char *argv[], char* fileNameSuffix, qA_t* fileQ, qB_t* dirQ);
int readOptArgs(int argc, char *argv[], int* directoryThreads, int* fileThreads, int* analysisThreads, char** fileNameSuffix);
int isReg(char* path);
int isDir(char* path);
int startsWith(char* str, char* prefix);
int endsWith(char* str, char* suffix);
void* fileThread(void *argptr);
void* dirThread(void *argptr);
void* analysisThread(void* argptr);
int fileWFD(char* filepath, fNode** WFDrepo);
char** getFileWords(FILE* fp, int* wordCount);
int calculateWFD(Node** head, int wordCount);
double calculateMeanFreq(Node* file1, Node* file2, char* word);
double calculateKLD(Node* calcFile, Node* suppFile);
double calculateJSD(Node* file1, Node* file2);
int compareWordCount(const void* pair1, const void* pair2);


int calculateWFD(Node** head, int wordCount) {
    double totalFreq = 0;

    Node* ptr = *head;
    while (ptr != NULL) {
        ptr->frequency = (ptr->count / (double) wordCount);
        totalFreq += ptr->frequency;
        ptr = ptr->next;
    }

    return EXIT_SUCCESS;
}

double calculateMeanFreq(Node* file1, Node* file2, char* word) {
    return 0.5 * (freqWord(file1, word) + freqWord(file2, word));
}

double calculateKLD(Node* calcFile, Node* suppFile) {
    double kldValue = 0;

    Node* ptr = calcFile;
    while (ptr != NULL) {
        double meanFrequency = calculateMeanFreq(calcFile, suppFile, ptr->word);
        kldValue += (ptr->frequency * log2(ptr->frequency / meanFrequency));
        ptr = ptr->next;
    }

    return kldValue;
}

double calculateJSD(Node* file1, Node* file2) {
    return sqrt(0.5 * calculateKLD(file1, file2) + 0.5 * calculateKLD(file2, file1));
}

typedef struct filepair {
    char* file1;
    char* file2;
    Node* file1Head;
    Node* file2Head;
    int totalWordCount;
    double JSD;
} filepair;

typedef struct file_arg {
    qA_t* fileQ;
    fNode** WFDrepo;
    int* activeThreads;
} f_arg;

typedef struct dir_arg {
    qB_t* dirQ;
    qA_t* fileQ;
    int* activeThreads;
    char* suffix;
} d_arg;

typedef struct analysis_arg {
    filepair** pairs;
    int startIndex;
    int numPairs;
} a_arg;

int main (int argc, char *argv[])
{
    int directoryThreads = 1;
    int fileThreads = 1;
    int analysisThreads = 1;

    char* defaultSuffix = ".txt";
    char* fileNameSuffix = malloc(sizeof(defaultSuffix) + 1);
    memcpy(fileNameSuffix, defaultSuffix, strlen(defaultSuffix));
    fileNameSuffix[strlen(defaultSuffix)] = '\0';

    int rc = EXIT_SUCCESS;

    rc = readOptArgs(argc, argv, &directoryThreads, &fileThreads, &analysisThreads, &fileNameSuffix);
    if (rc) {
        return rc;
    }

    qA_t fileQ;
    qB_t dirQ;
    startA(&fileQ);
    startB(&dirQ);

    fNode* WFDrepo = NULL;
    startFNode(WFDrepo);

    int activeThreads = directoryThreads;
    void* retval = NULL;

    readArgs(argc, argv, fileNameSuffix, &fileQ, &dirQ);

    pthread_t* dir_tids = malloc(directoryThreads * sizeof(pthread_t)); 
    d_arg* dir_args = malloc(directoryThreads * sizeof(d_arg)); 
    

    for (int i = 0; i < directoryThreads; i++) {
        dir_args[i].dirQ = &dirQ;
        dir_args[i].fileQ = &fileQ;
        dir_args[i].activeThreads = &activeThreads;
        dir_args[i].suffix = fileNameSuffix;
    }
    

    for (int i = 0; i < directoryThreads; i++) {
        pthread_create(&dir_tids[i], NULL, dirThread, &dir_args[i]);
    }


    pthread_t* file_tids = malloc(fileThreads * sizeof(pthread_t)); 
    f_arg* file_args = malloc(fileThreads * sizeof(f_arg)); 
    
    for (int i = 0; i < fileThreads; i++) {
        file_args[i].fileQ = &fileQ;
        file_args[i].WFDrepo = &WFDrepo;
        file_args[i].activeThreads = &activeThreads;
    }
    
    for (int i = 0; i < fileThreads; i++) {
        pthread_create(&file_tids[i], NULL, fileThread, &file_args[i]);
    }

    for (int i = 0; i < fileThreads; i++) {
        pthread_join(file_tids[i], &retval);
        if (*((int*)retval) == EXIT_FAILURE) {
            rc = EXIT_FAILURE;
        }
        free(retval);
    }

    for (int i = 0; i < directoryThreads; i++) {
        pthread_join(dir_tids[i], &retval);
        if (*((int*)retval) == EXIT_FAILURE) {
            rc = EXIT_FAILURE;
        }
        free(retval);
    }

    destroyB(&dirQ);
    destroyA(&fileQ);

    free(file_tids);
    free(dir_tids);
    free(file_args);
    free(dir_args);

    int n = fileListLength(WFDrepo);
    if (n < 2) {
        perror("Less than 2 files found.");
        return EXIT_FAILURE;
    }

    int combinations = n * (n - 1) / 2;
    filepair** pairs = malloc(combinations * sizeof(filepair*));

    int i = 0;
    for (fNode* ptr = WFDrepo; ptr != NULL; ptr = ptr->next) {
        for (fNode* ptr2 = ptr->next; ptr2 != NULL; ptr2 = ptr2->next) {
            filepair* pair = malloc(sizeof(filepair));
            pair->file1 = ptr->filename;
            pair->file2 = ptr2->filename;
            pair->file1Head = ptr->head;
            pair->file2Head = ptr2->head;
            pair->totalWordCount = ptr->wordCount + ptr2->wordCount;
            pair->JSD = 0;
            pairs[i] = pair;
            i++;
        }
    }

    int division = combinations / analysisThreads;
    int extraFiles = combinations % analysisThreads;
    int distributedThusFar = 0;

    pthread_t* analysis_tids = malloc(analysisThreads * sizeof(pthread_t)); 
    a_arg* analysis_args = malloc(analysisThreads * sizeof(a_arg)); 
    
    for (int i = 0; i < analysisThreads; i++) {
        analysis_args[i].pairs = pairs;
        analysis_args[i].numPairs = division;
        if (extraFiles > 0) {
            analysis_args[i].numPairs++;
            extraFiles--;
        }
        analysis_args[i].startIndex = distributedThusFar;
        distributedThusFar += analysis_args[i].numPairs;
    }
    
    
    for (int i = 0; i < analysisThreads; i++) {
        pthread_create(&analysis_tids[i], NULL, analysisThread, &analysis_args[i]);
    }

    for (int i = 0; i < analysisThreads; i++) {
        pthread_join(analysis_tids[i], &retval);
        free(retval);
    }
    
    free(analysis_tids);
    free(analysis_args);

    qsort(pairs, combinations, sizeof(filepair*), compareWordCount);

    for (int i = 0; i < combinations; i++) {
        printf("%f %s %s\n", pairs[i]->JSD, pairs[i]->file1, pairs[i]->file2);
        free(pairs[i]);
    }

    free(pairs);
    free(fileNameSuffix);
    freeFileList(WFDrepo);

    return rc;
}

int compareWordCount(const void* pair1, const void* pair2) {
    filepair** pairAPointer = (filepair**) (pair1);
    filepair** pairBPointer = (filepair**) (pair2);
    filepair* pairA = *pairAPointer;
    filepair* pairB = *pairBPointer;

    return pairB->totalWordCount - pairA->totalWordCount;
}

int readArgs (int argc, char *argv[], char* fileNameSuffix, qA_t* fileQ, qB_t* dirQ) {
    for (int i = 1; i < argc; i++) {
        if (startsWith(argv[i], "-") == 0) {
            continue;
        }
        else if (isReg(argv[i]) == 0) {
            if (endsWith(argv[i], fileNameSuffix)) {
                enqueueA(fileQ, argv[i]);
            }
        }
        else if (isDir(argv[i]) == 0) {
            enqueueB(dirQ, argv[i]);
        }
    }

    return EXIT_SUCCESS;
}

int readOptArgs (int argc, char *argv[], int* directoryThreads, int* fileThreads, int* analysisThreads, char** fileNameSuffix) {
    for (int i = 1; i < argc; i++) {
        if (startsWith(argv[i], "-") == 0) {
            int len = strlen(argv[i]) - 1;
            char* substring = malloc(len);
            memcpy(substring, argv[i] + 2, len);
            substring[len - 1] = '\0';

            if (startsWith(argv[i], "-s") == 0) {
                int suffixLen = strlen(substring) + 1;
                *fileNameSuffix = realloc(*fileNameSuffix, suffixLen);
                strcpy(*fileNameSuffix, substring);
            }
            else if (startsWith(argv[i], "-f") == 0) {
                *fileThreads = atoi(substring);
            }
            else if (startsWith(argv[i], "-d") == 0) {
                *directoryThreads = atoi(substring);
            }
            else if (startsWith(argv[i], "-a") == 0) {
                *analysisThreads = atoi(substring);
            }

            free(substring);
        }
        else if (isReg(argv[i]) && isDir(argv[i])) {
            perror("incorrect arguments");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int isReg (char* path) {
    struct stat arg_data;
    if (stat(path, &arg_data) == 0) {
        if (S_ISREG(arg_data.st_mode)) {
            return 0;
        }
    }
    else {
        return -1;
    }

    return 1;
}

int isDir (char* path) {
    struct stat arg_data;
    if (stat(path, &arg_data) == 0) {
        if (S_ISDIR(arg_data.st_mode)) {
            return 0;
        }
    }
    else {
        return -1; 
    }

    return 1;
}

int startsWith (char* str, char* prefix) {
    for (int i = 0; i < strlen(prefix); i++) {
        if (str[i] != prefix[i]) {
            return 1;
        }
    }

    return 0;
}

int endsWith (char* str, char* suffix) {
    if (strlen(suffix) > strlen(str)) { return 0; }

    return !strcmp(str + strlen(str) - strlen(suffix), suffix);
}

void* dirThread(void* argptr) {
    int* retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    d_arg* args = (d_arg*) argptr;

    qB_t* dirQ = args->dirQ;
    qA_t* fileQ = args->fileQ;
    int* activeThreads = args->activeThreads;
    char* suffix = args->suffix;

    while (dirQ->count > 0 || *activeThreads > 0) {
        if (dirQ->count == 0) {
            --(*activeThreads);
            if (*activeThreads <= 0) {
                pthread_cond_broadcast(&dirQ->read_ready);
                qcloseA(fileQ);
                return retval;
            }
            while (dirQ->count > 0 || *activeThreads > 0) {
                continue;
            }
            if (*activeThreads == 0) {
                return retval;
            }
        }

        char* dirPath;
        dequeueB(dirQ, &dirPath);
        
        int end = strlen(dirPath) - 1;
        if (dirPath[end] != '/') {
            int newLen = strlen(dirPath) + 2;
            char* dirPathNew = malloc(newLen);
            strcpy(dirPathNew, dirPath);
            dirPathNew[newLen - 2] = '/';
            dirPathNew[newLen - 1] = '\0';
            dirPath = realloc(dirPath, strlen(dirPathNew) + 1);
            strcpy(dirPath, dirPathNew);
            free(dirPathNew);
        }

        struct dirent* parentDirectory;
        DIR* parentDir;

        parentDir = opendir(dirPath);
        if (!parentDir) {
            perror("Failed to open directory!\n");
            *retval = EXIT_FAILURE;
            free(dirPath);
            return retval;
        }

        struct stat data;
        char* subpathname;
        char* subpath = malloc(0);

        while ((parentDirectory = readdir(parentDir))) {
            subpathname = parentDirectory->d_name;
            
            if (subpathname[0] != '.') {
                int subpath_size = strlen(dirPath)+strlen(subpathname) + 2;
                subpath = realloc(subpath, subpath_size * sizeof(char));
                strcpy(subpath, dirPath);
                
                strcat(subpath, subpathname);

                stat(subpath, &data);
                if (isReg(subpath) == 0 && endsWith(subpath, suffix)) {
                    enqueueA(fileQ, subpath);
                }
                else if (!isDir(subpath)) {
                    enqueueB(dirQ, subpath);
                }
            }
        }

        closedir(parentDir);
        free(subpath);
        free(dirPath);
    }

    return retval;
}

void* fileThread(void* argptr) {
    int* retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    f_arg* args = (f_arg*) argptr;

    qA_t* fileQ = args->fileQ;
    fNode** WFDrepo = args->WFDrepo;
    int* activeThreads = args->activeThreads;

    while (fileQ->count > 0 || *activeThreads > 0) {
        char* filepath = NULL;
        dequeueA(fileQ, &filepath);
        if (filepath != NULL) {
            fileWFD(filepath, WFDrepo);
        }
    }

    return retval;
}

void* analysisThread(void* argptr) {
    int* retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    a_arg* args = (a_arg*) argptr;

    filepair** pairs = args->pairs;
    int startIndex = args->startIndex;
    int numPairs = args->numPairs;

    for (int i = startIndex; i < numPairs + startIndex; i++) {
        filepair* pair = pairs[i];
        pair->JSD = calculateJSD(pair->file1Head, pair->file2Head);
    }

    return retval;
}

int fileWFD(char* filepath, fNode** WFDrepo) {
    Node* head = NULL;
    startHead(head);

    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("file opening failure");
        return EXIT_FAILURE;
    }

    int wordCount = 0;
    char** words = getFileWords(fp, &wordCount);
    for (int i = 0; i < wordCount; i++) {
        addNode(&head, words[i]);
    }

    for (int i = 0; i < wordCount; i++) {
        free(words[i]);
    }
    free(words);

    calculateWFD(&head, wordCount);

    insertFNode(WFDrepo, &head, filepath, wordCount);
    free(filepath);

    return EXIT_SUCCESS;
}

char** getFileWords(FILE* fp, int* wordCount) {
    char** words = malloc(0);

    char c;
    char* buf = malloc(5);
    strcpy(buf, "\0");
    int bufsize = 5;

    while ((c = fgetc(fp)) != EOF) {
        if (!isspace(c)) {
            if (!ispunct(c)) {
                size_t len = strlen(buf);
                if (len >= (bufsize - 1)){
                    buf = realloc(buf, len * 2);
                    bufsize = len * 2;
                }
                buf[len++] = c;
                buf[len] = '\0';
            }
        }
        else { 
            ++(*wordCount);

            int index = (*wordCount) - 1;
            words = realloc(words, (*wordCount) * sizeof(char*));
            words[index] = malloc(strlen(buf) + 2);

            for(int i = 0; buf[i]; i++){
                buf[i] = tolower(buf[i]);
            }
            strcpy(words[index], buf);
            strcat(words[index], "\0");

            buf = realloc(buf, 5);
            strcpy(buf, "\0");
            bufsize = 5;
        }
    }

    if (strcmp(buf, "\0") != 0) {
        ++(*wordCount);

        int index = (*wordCount) - 1;
        words = realloc(words, (*wordCount) * sizeof(char*));
        words[index] = malloc(strlen(buf) + 2);

        for(int i = 0; buf[i]; i++){
            buf[i] = tolower(buf[i]);
        }
        strcpy(words[index], buf);
        strcat(words[index], "\0");
    }

    free(buf);

    return words;
}

