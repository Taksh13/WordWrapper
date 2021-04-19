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


int optArg(char *str, char *prefix)
{
    for (int i = 0; i < strlen(prefix); i++)
    {
        if (str[i] != prefix[i])
        {
            return 1;
        }
    }

    return 0;
}

int lastChar(char *str, char *suffix)
{
    if (strlen(suffix) > strlen(str))
    {
        return 0;
    }

    return !strcmp(str + strlen(str) - strlen(suffix), suffix);
}

int isReg(char *path)
{
    struct stat arg_data;
    if (stat(path, &arg_data) == 0)
    {
        if (S_ISREG(arg_data.st_mode))
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }

    return 1;
}

int isDir(char *path)
{
    struct stat arg_data;
    if (stat(path, &arg_data) == 0)
    {
        if (S_ISDIR(arg_data.st_mode))
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }

    return 1;
}

int readArgs(int argc, char *argv[], char *fNameSuf, qA_t *f_Q, qB_t *d_Q);
int readOptArgs(int argc, char *argv[], int *dir_Threads, int *f_Threads, int *a_Threads, char **fNameSuf);




void *fThread(void *argptr);
void *dThread(void *argptr);

char **fRead(FILE *fp, int *wordCount);
int cmpWdCount(const void *pair1, const void *pair2);

int calcWFD(Node **head, int wordCount)
{
    double totalFreq = 0;

    Node *ptr = *head;
    while (ptr != NULL)
    {
        ptr->frequency = (ptr->count / (double)wordCount);
        totalFreq += ptr->frequency;
        ptr = ptr->next;
    }

    return EXIT_SUCCESS;
}

double calcMeanFreq(Node *file1, Node *file2, char *word)
{
    return 0.5 * (freqWord(file1, word) + freqWord(file2, word));
}

double calcKLD(Node *calcFile, Node *suppFile)
{
    double kldValue = 0;

    Node *ptr = calcFile;
    while (ptr != NULL)
    {
        double meanFrequency = calcMeanFreq(calcFile, suppFile, ptr->word);
        kldValue += (ptr->frequency * log2(ptr->frequency / meanFrequency));
        ptr = ptr->next;
    }

    return kldValue;
}

double calcJSD(Node *file1, Node *file2)
{
    return sqrt(0.5 * calcKLD(file1, file2) + 0.5 * calcKLD(file2, file1));
}

int WFD(char *filepath, fNode **WFDrepo)
{
    Node *head = NULL;
    startHead(head);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        perror("cannot open file\n");
        return EXIT_FAILURE;
    }

    int wordCount = 0;
    char **words = fRead(fp, &wordCount);
    for (int i = 0; i < wordCount; i++)
    {
        addNode(&head, words[i]);
    }

    for (int i = 0; i < wordCount; i++)
    {
        free(words[i]);
    }
    free(words);

    calcWFD(&head, wordCount);

    insertFNode(WFDrepo, &head, filepath, wordCount);
    free(filepath);

    return EXIT_SUCCESS;
}

//void *threadAnalysis(void *argptr);


typedef struct f_arg
{
    qA_t *f_Q;
    fNode **WFDrepo;
    int *actThreads;
} f_arg;

typedef struct f_pair
{
    char *file1;
    char *file2;
    Node *file1Head;
    Node *file2Head;
    int totalWordCount;
    double JSD;
} f_pair;

typedef struct a_arg
{
    f_pair **pairs;
    int startIndex;
    int numPairs;
} a_arg;

typedef struct d_arg
{
    qB_t *d_Q;
    qA_t *f_Q;
    int *actThreads;
    char *suffix;
} d_arg;

void *threadAnalysis(void *argptr)
{
    int *retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    a_arg *args = (a_arg *)argptr;

    f_pair **pairs = args->pairs;
    int startIndex = args->startIndex;
    int numPairs = args->numPairs;

    for (int i = startIndex; i < numPairs + startIndex; i++)
    {
        f_pair *pair = pairs[i];
        pair->JSD = calcJSD(pair->file1Head, pair->file2Head);
    }

    return retval;
}

int main(int argc, char *argv[])
{
    int dir_Threads = 1;
    int f_Threads = 1;
    int a_Threads = 1;

    char *defSuf = ".txt";
    char *fNameSuf = malloc(sizeof(defSuf) + 1);
    memcpy(fNameSuf, defSuf, strlen(defSuf));
    fNameSuf[strlen(defSuf)] = '\0';

    int rc = EXIT_SUCCESS;

    rc = readOptArgs(argc, argv, &dir_Threads, &f_Threads, &a_Threads, &fNameSuf);
    if (rc)
    {
        return rc;
    }

    qA_t f_Q;
    qB_t d_Q;
    startA(&f_Q);
    startB(&d_Q);

    fNode *WFDrepo = NULL;
    startFNode(WFDrepo);

    int actThreads = dir_Threads;
    void *retval = NULL;

    readArgs(argc, argv, fNameSuf, &f_Q, &d_Q);

    pthread_t *dir_tids = malloc(dir_Threads * sizeof(pthread_t));
    d_arg *dir_args = malloc(dir_Threads * sizeof(d_arg));

    for (int i = 0; i < dir_Threads; i++)
    {
        dir_args[i].d_Q = &d_Q;
        dir_args[i].f_Q = &f_Q;
        dir_args[i].actThreads = &actThreads;
        dir_args[i].suffix = fNameSuf;
    }

    for (int i = 0; i < dir_Threads; i++)
    {
        pthread_create(&dir_tids[i], NULL, dThread, &dir_args[i]);
    }

    pthread_t *file_tids = malloc(f_Threads * sizeof(pthread_t));
    f_arg *file_args = malloc(f_Threads * sizeof(f_arg));

    for (int i = 0; i < f_Threads; i++)
    {
        file_args[i].f_Q = &f_Q;
        file_args[i].WFDrepo = &WFDrepo;
        file_args[i].actThreads = &actThreads;
    }

    for (int i = 0; i < f_Threads; i++)
    {
        pthread_create(&file_tids[i], NULL, fThread, &file_args[i]);
    }

    for (int i = 0; i < f_Threads; i++)
    {
        pthread_join(file_tids[i], &retval);
        if (*((int *)retval) == EXIT_FAILURE)
        {
            rc = EXIT_FAILURE;
        }
        free(retval);
    }

    for (int i = 0; i < dir_Threads; i++)
    {
        pthread_join(dir_tids[i], &retval);
        if (*((int *)retval) == EXIT_FAILURE)
        {
            rc = EXIT_FAILURE;
        }
        free(retval);
    }

    destroyB(&d_Q);
    destroyA(&f_Q);

    free(file_tids);
    free(dir_tids);
    free(file_args);
    free(dir_args);

    int n = fileListLength(WFDrepo);
    if (n < 2)
    {
        perror("Only one file found\n");
        return EXIT_FAILURE;
    }

    int combinations = n * (n - 1) / 2;
    f_pair **pairs = malloc(combinations * sizeof(f_pair *));

    int i = 0;
    for (fNode *ptr = WFDrepo; ptr != NULL; ptr = ptr->next)
    {
        for (fNode *ptr2 = ptr->next; ptr2 != NULL; ptr2 = ptr2->next)
        {
            f_pair *pair = malloc(sizeof(f_pair));
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

    int division = combinations / a_Threads;
    int extraFiles = combinations % a_Threads;
    int distributedThusFar = 0;

    pthread_t *analysis_tids = malloc(a_Threads * sizeof(pthread_t));
    a_arg *analysis_args = malloc(a_Threads * sizeof(a_arg));

    for (int i = 0; i < a_Threads; i++)
    {
        analysis_args[i].pairs = pairs;
        analysis_args[i].numPairs = division;
        if (extraFiles > 0)
        {
            analysis_args[i].numPairs++;
            extraFiles--;
        }
        analysis_args[i].startIndex = distributedThusFar;
        distributedThusFar += analysis_args[i].numPairs;
    }

    for (int i = 0; i < a_Threads; i++)
    {
        pthread_create(&analysis_tids[i], NULL, threadAnalysis, &analysis_args[i]);
    }

    for (int i = 0; i < a_Threads; i++)
    {
        pthread_join(analysis_tids[i], &retval);
        free(retval);
    }

    free(analysis_tids);
    free(analysis_args);

    qsort(pairs, combinations, sizeof(f_pair *), cmpWdCount);

    for (int i = 0; i < combinations; i++)
    {
        printf("%f %s %s\n", pairs[i]->JSD, pairs[i]->file1, pairs[i]->file2);
        free(pairs[i]);
    }

    free(pairs);
    free(fNameSuf);
    freeFileList(WFDrepo);

    return rc;
}

int cmpWdCount(const void *pair1, const void *pair2)
{
    f_pair **pairAPointer = (f_pair **)(pair1);
    f_pair **pairBPointer = (f_pair **)(pair2);
    f_pair *pairA = *pairAPointer;
    f_pair *pairB = *pairBPointer;

    return pairB->totalWordCount - pairA->totalWordCount;
}

int readArgs(int argc, char *argv[], char *fNameSuf, qA_t *f_Q, qB_t *d_Q)
{
    for (int i = 1; i < argc; i++)
    {
        if (optArg(argv[i], "-") == 0)
        {
            continue;
        }
        else if (isReg(argv[i]) == 0)
        {
            if (lastChar(argv[i], fNameSuf))
            {
                enqueueA(f_Q, argv[i]);
            }
        }
        else if (isDir(argv[i]) == 0)
        {
            enqueueB(d_Q, argv[i]);
        }
    }

    return EXIT_SUCCESS;
}

int readOptArgs(int argc, char *argv[], int *dir_Threads, int *f_Threads, int *a_Threads, char **fNameSuf)
{
    for (int i = 1; i < argc; i++)
    {
        if (optArg(argv[i], "-") == 0)
        {
            int len = strlen(argv[i]) - 1;
            char *substring = malloc(len);
            memcpy(substring, argv[i] + 2, len);
            substring[len - 1] = '\0';

            if (optArg(argv[i], "-s") == 0)
            {
                int suffixLen = strlen(substring) + 1;
                *fNameSuf = realloc(*fNameSuf, suffixLen);
                strcpy(*fNameSuf, substring);
            }
            else if (optArg(argv[i], "-f") == 0)
            {
                *f_Threads = atoi(substring);
            }
            else if (optArg(argv[i], "-d") == 0)
            {
                *dir_Threads = atoi(substring);
            }
            else if (optArg(argv[i], "-a") == 0)
            {
                *a_Threads = atoi(substring);
            }

            free(substring);
        }
        else if (isReg(argv[i]) && isDir(argv[i]))
        {
            perror("wrong args\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}






void *dThread(void *argptr)
{
    int *retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    d_arg *args = (d_arg *)argptr;

    qB_t *d_Q = args->d_Q;
    qA_t *f_Q = args->f_Q;
    int *actThreads = args->actThreads;
    char *suffix = args->suffix;

    while (d_Q->count > 0 || *actThreads > 0)
    {
        if (d_Q->count == 0)
        {
            --(*actThreads);
            if (*actThreads <= 0)
            {
                pthread_cond_broadcast(&d_Q->read_ready);
                qcloseA(f_Q);
                return retval;
            }
            while (d_Q->count > 0 || *actThreads > 0)
            {
                continue;
            }
            if (*actThreads == 0)
            {
                return retval;
            }
        }

        char *dirPath;
        dequeueB(d_Q, &dirPath);

        int end = strlen(dirPath) - 1;
        if (dirPath[end] != '/')
        {
            int newLen = strlen(dirPath) + 2;
            char *dirPathNew = malloc(newLen);
            strcpy(dirPathNew, dirPath);
            dirPathNew[newLen - 2] = '/';
            dirPathNew[newLen - 1] = '\0';
            dirPath = realloc(dirPath, strlen(dirPathNew) + 1);
            strcpy(dirPath, dirPathNew);
            free(dirPathNew);
        }

        struct dirent *parentDirectory;
        DIR *parentDir;

        parentDir = opendir(dirPath);
        if (!parentDir)
        {
            perror("cannot open dir\n");
            *retval = EXIT_FAILURE;
            free(dirPath);
            return retval;
        }

        struct stat data;
        char *subpathname;
        char *subpath = malloc(0);

        while ((parentDirectory = readdir(parentDir)))
        {
            subpathname = parentDirectory->d_name;

            if (subpathname[0] != '.')
            {
                int subpath_size = strlen(dirPath) + strlen(subpathname) + 2;
                subpath = realloc(subpath, subpath_size * sizeof(char));
                strcpy(subpath, dirPath);

                strcat(subpath, subpathname);

                stat(subpath, &data);
                if (isReg(subpath) == 0 && lastChar(subpath, suffix))
                {
                    enqueueA(f_Q, subpath);
                }
                else if (!isDir(subpath))
                {
                    enqueueB(d_Q, subpath);
                }
            }
        }

        closedir(parentDir);
        free(subpath);
        free(dirPath);
    }

    return retval;
}



void *fThread(void *argptr)
{
    int *retval = malloc(sizeof(int));
    *retval = EXIT_SUCCESS;

    f_arg *args = (f_arg *)argptr;

    qA_t *f_Q = args->f_Q;
    fNode **WFDrepo = args->WFDrepo;
    int *actThreads = args->actThreads;

    while (f_Q->count > 0 || *actThreads > 0)
    {
        char *filepath = NULL;
        dequeueA(f_Q, &filepath);
        if (filepath != NULL)
        {
            WFD(filepath, WFDrepo);
        }
    }

    return retval;
}


char **fRead(FILE *fp, int *wordCount)
{
    char **words = malloc(0);

    char c;
    char *buf = malloc(5);
    strcpy(buf, "\0");
    int bufsize = 5;

    while ((c = fgetc(fp)) != EOF)
    {
        if (!isspace(c))
        {
            if (!ispunct(c))
            {
                size_t len = strlen(buf);
                if (len >= (bufsize - 1))
                {
                    buf = realloc(buf, len * 2);
                    bufsize = len * 2;
                }
                buf[len++] = c;
                buf[len] = '\0';
            }
        }
        else
        {
            ++(*wordCount);

            int index = (*wordCount) - 1;
            words = realloc(words, (*wordCount) * sizeof(char *));
            words[index] = malloc(strlen(buf) + 2);

            for (int i = 0; buf[i]; i++)
            {
                buf[i] = tolower(buf[i]);
            }
            strcpy(words[index], buf);
            strcat(words[index], "\0");

            buf = realloc(buf, 5);
            strcpy(buf, "\0");
            bufsize = 5;
        }
    }

    if (strcmp(buf, "\0") != 0)
    {
        ++(*wordCount);

        int index = (*wordCount) - 1;
        words = realloc(words, (*wordCount) * sizeof(char *));
        words[index] = malloc(strlen(buf) + 2);

        for (int i = 0; buf[i]; i++)
        {
            buf[i] = tolower(buf[i]);
        }
        strcpy(words[index], buf);
        strcat(words[index], "\0");
    }

    free(buf);

    return words;
}
