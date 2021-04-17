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

void freelist(Node *head);
void printlist(Node *head);

#endif