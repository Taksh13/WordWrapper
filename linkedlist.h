#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

typedef struct Node
{
    char *word;
    struct Node *next;
    double frequency;
    int count;
} Node;

int startHead(Node *head);
int addNode(Node **head, char *word);
double freqWord(Node *head, char *word);

void freeList(Node *head);
void printList(Node *head);

#endif