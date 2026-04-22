#include <string.h>
#include <sys/wait.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

struct processNode {
  pid_t pid;
  char* program;
  struct processNode* next;
};
struct processNode* head = NULL;
struct processNode* tail = NULL;
void signal_handler(int signum){
  pid_t childpid;
  struct processNode* node;
  struct processNode* prevNode;
  while ((childpid = waitpid(-1, NULL, WNOHANG)) > 0) {
    if(head != NULL){
      prevNode = head;
      if(head->pid == childpid){
        head = head->next;
        free(prevNode->program);
        free(prevNode);
        prevNode = NULL;
      } else {
        while(prevNode->next != NULL){
          node = prevNode->next;
          if(node->pid == childpid){
            if(node == tail)
              tail = prevNode;
            prevNode->next = node->next;
            free(node->program);
            free(node);
            node = NULL;
            break;
          }
          prevNode = prevNode->next;
        }
      }
    }
  }
}
pid_t externalBackgroundCommand(char* program, char** arg_list, int numArgs) {
  pid_t child_pid; 
  char* child_args[51];
  for(int i = 0; i < numArgs; i++) {
    child_args[i] = arg_list[i];
  }
  child_args[numArgs] = NULL;
  child_pid = fork();
  if (child_pid != 0) {
    for(int i = 0; i < numArgs; i++)
      arg_list[i] = NULL;
    return child_pid; 
  } else {
      execvp (program, child_args);
      fprintf (stderr, "an error occurred in execvp\n");
      abort ();
  }
}
pid_t externalForegroundCommand(char* program, char** args, int numArgs) {
  pid_t childpid = fork();
  if (childpid == 0){
    int status_code = execvp(program, args);
    if (status_code == -1){
      printf("Process did not terminate correctly\n");
      for(int i = 0; i < numArgs; i++){
        free(args[i]);
        args[i] = NULL;
      }
      exit(1);
    }
    printf("This line will not be printed if execvp() runs correctly\n");
  } else {
    return childpid;
  }
}
char* subString(char string[], int startIndex, int endIndex){
  int stringSize = (endIndex - startIndex + 1);
  char* returnString = (char*) malloc((stringSize+1) * sizeof(char));
  for(int i = 0; i < stringSize; i++) {
    returnString[i] = string[startIndex + i];
  }
  returnString[stringSize] = '\0';
  return returnString;
}

int main(int argc, char *argv[])
  {
  struct processNode* traverseNode = NULL;
  char  line[100];
  char* arg_list[51] = {NULL};
  char  character;
  char* program;
  int   index;
  int   prevIndex;
  int   numArgs;
  pid_t waitPid;
  pid_t childpid;
  int   status;
  sigset_t blockMask;
  sigemptyset(&blockMask);
  if(sigaddset(&blockMask, SIGCHLD) == -1)
    perror("set signal mask");
  if (signal(SIGCHLD, signal_handler) == SIG_ERR){
    perror("signal");
    exit(1);
  }
  while (1)
    {
    printf("tish>> "); // sets the command prompt
    fgets(line, 100, stdin); // gets the user input

		//This here will be for internal commands
    //Bye
    line[strcspn(line, "\n")] = '\0';
    if(strcmp(line, "bye") == 0){
      if(sigprocmask(SIG_BLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      traverseNode = head;
      while(traverseNode != NULL){
        kill(traverseNode->pid, SIGTERM);
        traverseNode = traverseNode->next;
      }
      if(sigprocmask(SIG_UNBLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      exit(0);
    }
    //Jobs
    else if(strcmp(line, "jobs") == 0){
      if(sigprocmask(SIG_BLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      traverseNode = head;
      while(traverseNode != NULL){
        printf("%d %s\n", traverseNode->pid, traverseNode->program);
        traverseNode = traverseNode->next;
      }
      if(sigprocmask(SIG_UNBLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
    }
		//
    //Parse to create args list
    else{
    program = NULL;
    index = 0;
    prevIndex = index;
    numArgs = 0;
    if(sigprocmask(SIG_BLOCK, &blockMask, 0) == -1)
      perror("set signal mask");
    while(1){
      character = line[index];
      if(character == '\t' || character == ' ' || character == '\0'){
       if(prevIndex != index){
          arg_list[numArgs] = subString(line, prevIndex, index-1);
          if(numArgs == 0){
            program = (char *)malloc((index-prevIndex+1)*sizeof(char));
            int ind = 0;
            strcpy(program, arg_list[0]);
          }
          numArgs++;
        }
        prevIndex = index + 1;
      }
      if(character == '\0')
        break;
      index++;
    }
    if(sigprocmask(SIG_UNBLOCK, &blockMask, 0) == -1)
      perror("set signal mask");
    //kill
    if(numArgs == 2 && strcmp(arg_list[0], "kill") == 0){
      if(sigprocmask(SIG_BLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      pid_t pid_to_kill = (pid_t) atoi(arg_list[1]);
      traverseNode = head;
      while(traverseNode != NULL){
        if(traverseNode->pid == pid_to_kill){
          kill(pid_to_kill, SIGTERM);
          break;
        }
        traverseNode = traverseNode->next;
      }
      if(sigprocmask(SIG_UNBLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
    }
    //Execute command on foreground
    else if(strcmp(arg_list[numArgs-1], "&") != 0){
      childpid = externalForegroundCommand(program, arg_list, numArgs);
      free(program);
      waitPid = waitpid(childpid, &status, 0);
      for(int i = 0; i < numArgs; i++){
        free(arg_list[i]);
        arg_list[i] = NULL;
      }
    }
    //Execute command on background
    else {
      numArgs--;
      free(arg_list[numArgs]);
      arg_list[numArgs] = NULL;
      childpid = externalBackgroundCommand(program, arg_list, numArgs);
      if(sigprocmask(SIG_BLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      if(head == NULL){
        head = (struct processNode*) malloc(sizeof(struct processNode));
        head->program = program;
        head->pid = childpid;
        head->next = NULL;
        tail = head;
      } else {
        tail->next = (struct processNode*) malloc(sizeof(struct processNode));
        tail = tail->next;
        tail->program = program;
        tail->pid = childpid;
        tail->next = NULL;
      }
      if(sigprocmask(SIG_UNBLOCK, &blockMask, 0) == -1)
        perror("set signal mask");
      }
    }
  }
  return 0;
} 
