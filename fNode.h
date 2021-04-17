#include "linkedlist.h"

typedef struct fNode
{
    Node* head;
    struct fNode* next;
    char* filename;
    int wordCount;
} fNode;

int startFNode (fNode* fNodeHead);
int insertFNode(fNode** fNodeHead, Node** head, char* filename, int wordCount);
void printFileList(fNode* fNodeHead);
void freeFileList(fNode* fNodeHead);
int fileListLength(fNode* fNodeHead);