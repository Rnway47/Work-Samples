#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

typedef struct Node Node;

struct Node{
    pid_t pid;
    char command[200]; //input command or running program name
    Node * next;
};


void add_newNode(Node** head, pid_t new_pid, char *new_command); //add a new node, use with bg
void deleteNode(Node** head, pid_t pid); //delete a node, use with bgkill
void printList(Node *node); //print everything in the linked list
int check_pid(Node *head, pid_t pid_check); //check if process ID still exists
int checksize(Node *head); //check the size of the linked list (total background jobs)
pid_t get_pid(Node *head, char *str_pid); //find the corresponding process ID in pid_t form


#endif