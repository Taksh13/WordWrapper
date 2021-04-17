#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linkedlist.h"

int startHead(Node *head)
{
    head = NULL;
    return EXIT_SUCCESS;
}

int addNode(Node **head, char *word)
{
    Node *nNode = malloc(sizeof(Node));
    int len = strlen(word) + 1;
    nNode->word = malloc(len);
    memcpy(nNode->word, word, len);
    nNode->next = NULL;
    nNode->count = 1;

    if ((*head) == NULL)
    {
        *head = nNode;
        return 0;
    }
    else if (strcmp((*head)->word, word) > 0)
    {
        nNode->next = *head;
        *head = nNode;
        return 0;
    }
    else
    {
        Node *current = *head;
        Node *prev = *head;
        while (current != NULL)
        {
            if (strcmp(current->word, word) == 0)
            {
                free(nNode->word);
                free(nNode);
                ++current->count;
                return 0;
            }
            else if (strcmp(current->word, word) > 0)
            {
                nNode->next = current;
                prev->next = nNode;
                return 0;
            }
            prev = current;
            current = current->next;
        }
        prev->next = nNode;
    }

    return 0;
}

double freqWord(Node *head, char *word)
{
    Node *ptr = head;
    while (ptr != NULL)
    {
        if (strcmp(ptr->word, word) == 0)
        {
            return ptr->frequency;
        }
        ptr = ptr->next;
    }

    return 0;
}


void freelist(Node *head)
{
    Node *tempNode = head;
    while (head != NULL)
    {
        tempNode = head;
        head = head->next;
        free(tempNode->word);
        free(tempNode);
    }
}

void printlist(Node *head)
{
    Node *ptr = head;
    while (ptr != NULL)
    {
        printf("%s: %d\t", ptr->word, ptr->count);
        ptr = ptr->next;
    }
    printf("\n");
}