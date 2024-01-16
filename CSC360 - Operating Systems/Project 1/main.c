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
struct Node *head = NULL;

int check_command(char *cmd){ //check if it's a executable command
  char path[200];
  FILE *command = popen(cmd, "r");
  char *file_name = strtok(cmd, "./");
  if(!access(file_name, F_OK)){ //checking if it's a executable file
      return 0;    
  }else if(command){ //checking if it's a linux command
    if(fgets(path, sizeof(path), command) != NULL){
      return 0;
    }
  }
  return 1;
}

void func_BG(char *cmd, Node **head){ //start a new process with the input command name if it can be found
  int check = check_command(cmd);
  pid_t pid;
  int status;
  int opts = WUNTRACED;
  int retVal;
  if (cmd == NULL || check == 1){ //handles the error of input command for "bg"
    printf("Invalid command, please input a valid command\n");
  }else{
  pid = fork(); 
  if(pid == 0){ //child process
    char *argv_execvp[] = {cmd, NULL};
    if(execvp(cmd, argv_execvp) < 0){ //error handling of execvp
      printf("Error running execvp");
      exit(EXIT_FAILURE);
    }
  }else if(pid > 0){
    retVal = waitpid(pid, &status, opts); //wait for child process to finish
    if (retVal == -1){ 
		  perror("waitpid failed\n"); 
		  exit(EXIT_FAILURE);
    }else{
      add_newNode(head, pid, cmd); //record the successful process creation in the linked list
    }
  }else{
      printf("Process creation failed\n"); //fork failed
      exit(1);
  }
  }
  setbuf(stdin, NULL);
}


void func_BGlist(Node *head){ //list all the running and stopped processes
  int size = 0;
  printList(head);        //print the process that are not terminated
  size = checksize(head); //checking the total background processes
  printf("Total background jobs:  %d\n", size);  
}


void func_BGkill(char * str_pid, Node **head){ //kill a process based on the PIDs and remove the process from the linkedlist
  pid_t pid = get_pid(*head, str_pid);
  int check = check_pid(*head, pid);
  if(check == 1 || str_pid == NULL){ //check if the process exists
    printf("Please input a valid Process ID\n");
  }else{
    if(kill(pid, SIGTERM) == -1){ //error handling of kill()
      printf("Error running kill(pid_t pid, SIGTERM) command\n");
    }else{
      printf("Process has been terminated\n");
      deleteNode(head, pid); //remove it from the linked list
    }
  }
  setbuf(stdin, NULL);
}


void func_BGstop(char * str_pid, Node **head){
	pid_t pid = get_pid(*head, str_pid);
  int check = check_pid(*head, pid);
  if(check == 1 || str_pid == NULL){
    printf("Please input a valid Process ID\n");//check if the process exists
  }else{
    if(kill(pid, SIGSTOP) == -1){ //error handling of kill()
      printf("Error running kill(pid_t pid, SIGSTOP) command\n");
    }
  }
  setbuf(stdin, NULL);
}


void func_BGstart(char * str_pid, Node **head){
	pid_t pid = get_pid(*head, str_pid);
  int check = check_pid(*head, pid);
  if(check == 1 || str_pid == NULL){
    printf("Please input a valid Process ID\n"); //check if the process exists
  }else{
    if(kill(pid, SIGCONT) == -1){
      printf("Error running kill(pid_t pid, SIGCONT) command\n"); //error handling of kill()
    }
  }
  setbuf(stdin, NULL);
}


void func_pstat(char * str_pid, Node *head){
  int count_string = 0;
  int count_char = 0;
  int count_unsigned_long = 0;
  int count_long_int = 0;
	int pid = atoi(str_pid);
  int check = check_pid(head, pid);
  char comm[200]; //comm
  char state; //process state
  unsigned long utime; //utime
  long int rss; //rss
  FILE *file;
  char index[100] = "/proc/";
  char append[100] = "/stat";
  char file_name[100];
  if(check == 1 || str_pid == NULL){ //check if the process ID still exists
    printf("Process ID doesn't exist, please check your input");
  }else{
    strcat(file_name, index);
    strcat(file_name, str_pid);
    strcat(file_name, append); //make it /proc/PID/stat
    file = fopen(file_name, "r");
    while(file != NULL){
      fscanf(file, "%s", &comm [0]);
      if(comm != NULL && count_string == 0){ //find the filename 
        printf("Comm: %s\n", comm);
        count_string++;
      }
      fscanf(file, "%c", &state);
      if(state != NULL && count_char == 0){ //find the state of the process
        printf("State: %c\n", state);
        count_char++;
      }
      fscanf(file, "%lu", &utime);
      if(utime != NULL && count_unsigned_long == 12){ //find utime
        printf("utime: %lu\n", utime);
        count_unsigned_long++;
      }else if(utime != NULL && count_unsigned_long == 13){ //find stime
        printf("stime: %lu\n", utime);
        count_unsigned_long++;
      }else if(utime != NULL){ //keep looping until utime is reached
        count_unsigned_long++;
      }
      fscanf(file, "%ld", &rss); 
      if(rss != NULL && count_unsigned_long == 21){ //find rss
        printf("rss: %ld\n", rss);
      }else if(rss != NULL){ //keeping looping until rss is found
        count_long_int++;
      }
    }
  }
}


 
int main(){
    char user_input_str[50];
    while (true){
      printf("Pman: > ");
      fgets(user_input_str, 50, stdin);
      char * ptr = strtok(user_input_str, " \n");
      if(ptr == NULL){
        continue;
      }
      char * lst[50];
      int index = 0;
      lst[index] = ptr;
      index++;
      while(ptr != NULL){
        ptr = strtok(NULL, " \n");
        lst[index]=ptr;
        index++;
      }
      if (strcmp("bg",lst[0]) == 0){
        func_BG(lst[1], &head);
      } else if (strcmp("bglist",lst[0]) == 0) {
        func_BGlist(head);
      } else if (strcmp("bgkill",lst[0]) == 0) {
        func_BGkill(lst[1], &head);
      } else if (strcmp("bgstop",lst[0]) == 0) {
        func_BGstop(lst[1], &head);
      } else if (strcmp("bgstart",lst[0]) == 0) {
        func_BGstart(lst[1], &head);
      } else if (strcmp("pstat",lst[0]) == 0) { 
        func_pstat(lst[1], head);
      } else if (strcmp("q",lst[0]) == 0) { //quit pman
        printf("Bye Bye \n");
        exit(0);
      } else {
        printf("Invalid input\n"); //return this if none of the input command is found
      }
    }

  return 0;
}

