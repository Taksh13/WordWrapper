#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fNode.h"

int startFNode (fNode* fNodeHead) {
    fNodeHead = NULL;

    return EXIT_SUCCESS;
}
int insertFNode(fNode** fNodeHead, Node** head, char* filename, int wordCount) {
    fNode* newNode = malloc(sizeof(fNode));
    newNode->head = *head;
    int len = strlen(filename) + 1;
    newNode->filename = malloc(len);
    memcpy(newNode->filename, filename, len);
    newNode->next = NULL;
    newNode->wordCount = wordCount;

    if ((*fNodeHead) == NULL) {
        *fNodeHead = newNode;
        return 0;
    }
    else if (strcmp((*fNodeHead)->filename, filename) > 0) {
        newNode->next = *fNodeHead;
        *fNodeHead = newNode;
        return 0;
    }
    else {
        fNode* current = *fNodeHead;
        fNode* prev = *fNodeHead;
        while (current != NULL){
            if (strcmp(current->filename, filename) > 0) {
                newNode->next = current;
                prev->next = newNode;
                return 0;
            }
            prev = current;
            current = current->next;
        }
        prev->next = newNode;
    }

    return 0;
}

void printFileList(fNode* fNodeHead) {
    fNode* ptr = fNodeHead;
    while (ptr != NULL) {
        printf("%s: %d\t", ptr->filename, ptr->wordCount);
        ptr = ptr->next;
    }
    printf("\n");
}

void freeFileList(fNode* fNodeHead) {
    fNode* tempNode = fNodeHead;
    while (fNodeHead != NULL) {
        tempNode = fNodeHead;
        fNodeHead = fNodeHead->next;
        free(tempNode->filename);
        freeList(tempNode->head);
        free(tempNode);
    }
}

int fileListLength(fNode* fNodeHead) {
    int len = 0;
    fNode* ptr = fNodeHead;
    while (ptr != NULL) {
        len++;
        ptr = ptr->next;
    }

    return len;
}