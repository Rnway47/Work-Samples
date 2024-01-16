#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"
 

void add_newNode(struct Node **head, pid_t new_pid, char *new_command){ //borrow ideas from https://www.log2base2.com/data-structures/linked-list/inserting-a-node-at-the-end-of-a-linked-list.html 
	struct Node *New_Node = malloc(sizeof(struct Node));
	struct Node *Last_Node = *head; 
	New_Node->pid = new_pid;
	strncpy(New_Node->command, new_command, 100);
	New_Node->next = NULL; //this will remain the same if it is run on a different user/computer; I have tried to use getcwd but it would fail in bglist
	if (*head == NULL){  //checking if it's a empty linkedlist
		*head = New_Node;
	}else{
		while(Last_Node->next != NULL){
			Last_Node = Last_Node->next; //iterate until reaching the last node of the linked list
		}
		Last_Node->next = New_Node;
	}
}

void deleteNode(Node** head, pid_t dead_pid){
	struct Node *temp = *head;
	struct Node *prev;
	if (temp->pid == dead_pid){ //checking if the head node is the one needs to be deleted
		*head = temp->next;
		free(temp); 
	}
	while (temp != NULL && temp->pid != dead_pid){ //delete the node that has the dead pid and relink the nodes
		prev = temp;
		temp = temp->next;
	}
	prev->next = temp->next;
	free(temp);
	//we will ignore the case where the head node is null since we will call it when a process is terminated 
}

void printList(Node *node){ //the input will be the head of the linked list, print out everything in the linked list
	while(node != NULL){
		printf("%d: %s\n", node->pid, node->command); 
		node = node->next;
	}
}

int check_pid(Node *head, pid_t pid_check){ //check if pid exists or not
	while(head != NULL){
		if(head->pid == pid_check){
			return 0;
		}else{
			head = head->next;
		}
	}
	return 1;
}

int checksize(Node *head){ //return the size of the linked list
	int size = 0;
	while(head != NULL){
		head = head->next;
		size++;
	}
	return size;
}

pid_t get_pid(Node *head, char *str_pid){ //we won't check if the pid exists or not, only use the returned Process ID after checking if the process ID exists
	int int_pid = atoi(str_pid);
	while(head != NULL){
		if(int_pid == head->pid){
			return head->pid;
		}else{
			head = head->next;
		}
	}
	return 0;
}
