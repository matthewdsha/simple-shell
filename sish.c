#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>     // isdigit
#include <unistd.h>    // chdir, execvp, fork
#include <sys/wait.h>  // wait
#include <sys/types.h> // pid_t

// define global variables
#define SIZE 100

// check if history command argument is a number
int isNumber(char *arg) {
  int i;
  // check if each character in arg is a digit
  for (i = 0; i < strlen(arg); i++)
    // if character is not a digit, return 0 (false)
    if (!isdigit(arg[i]))
      return 0;
  // if all characters are digits, return 1 (true)
  return 1;
}

// adding element to queue, pass in history queue, head & tail indices, argument string
static void enqueue(char *history[SIZE], int *head, int *tail, char *cmd) {
  // move tail index to next index
  *tail = (*tail + 1) % SIZE;
  // if queue is empty (*head == -1), set head to next index (0)
  if (*head == -1) {
    *head = (*head + 1) % SIZE;
  }
  // if queue is full (tail is pointing to same index as head), move head to next index
  else if (*head == *tail) {
    // only moves head index when queue is full
    *head = (*head + 1) % SIZE;
  }
  // free memory at tail if element not null
  if (history[*tail] != NULL) {
    free(history[*tail]);
  }
  // set contents at tail index to command string
  history[*tail] = (char*)malloc((strlen(cmd) + 1) * sizeof(char));
  strcpy(history[*tail], cmd);
}

// execute history command based on input argument (NULL - print, "-c" - clear)
void executeHistory(char *history[], char *arg, int *head, int *tail) {
  // i iterates through queue, j is loop counter for history list display
  int i, j;
  // if argument is empty (NULL), print all commands in history
  if (!arg) {
    // validation: if history is empty, return
    // note: history called with no argument should print history
    if (history[0] == NULL) {
      return;
    }
    // iterate through circular queue with i, loop with j
    for (i = *head, j = 0; j < SIZE; i = (i + 1) % SIZE, j++) {
      // print index of history and corresponding element in queue
      printf("%d %s\n", j, history[i]);
      // if i is equal to tail (last element printed), break loop
      // i = (i + 1) % SIZE could cause infinite loop
      if (i == *tail)
        break;
    }
    return;
  }
  // if argument is "-c", clear contents of history queue (set to NULL)
  if (strcmp(arg, "-c") == 0) {
    // loop through each element that is not NULL and set to NULL
    for (i = 0; i < SIZE; i++)
      history[i] = NULL;
    // set head and tail indices to -1 (empty circular queue)
    *head = -1;
    *tail = -1;
    return;
  }
  // if argument invalid, print error message
  else
    perror("Invalid argument for history.");
}

int main(int argc, char *argv[]) {
  // pointers for tokenizing
  char *input = NULL, *command = NULL, *token = NULL, *saveptr = NULL, *tmp = NULL;
  // pointers to arrays for tokenizing, history, execvp purposes
  char *args[SIZE], *cmds[SIZE], *tokens[SIZE], *history[SIZE];
  // buffer for getline
  size_t buf = 0;
  // variables to track indices, queue head/tail, pipe status
  int i, j, k, l, head, tail, queued, pipefd1[2], pipefd2[2];
  // variables to track number of commands/arguments, loop condition
  int cmdcount = 0, argcount = 0, numargs = 0, execvpCondition = 0;
  // variable to indicate child process
  pid_t cpid;
  
  // set circular queue head and tail indices to -1
  head = -1, tail = -1;
  // set all elements in history to NULL
  for (i = 0; i < SIZE; i++) {
    history[i] = NULL;
  }
  
  // loop sish> prompt until exit
  while (1) {
    // if condition to implement execvp is not true
    if (!execvpCondition) {
      queued = 0;
      // print sish> prompt
      printf("sish> ");
      // get input from user
      getline(&input, &buf, stdin);
      // tokenize input string by newline delimiter
      input = strtok(input, "\n");
      // we then store input into a tmp pointer so that way we can use it to enqueue into history later while we tokenize input
      tmp =  (char*)malloc(strlen(input) + 1);
      strncpy(tmp, input, strlen(input));
      tmp = strtok(tmp, "\n");
    }
    // if condition to implement execvp is true, loop successful, reset condition to 0 (false)
    else 
      execvpCondition = 0;
    
    // tokenize command by pipe delimiter
    command = strtok(input, "|");
    // set all elements in arrays to NULL
    for (i = 0; i < SIZE; i++)
      args[i] = NULL;
    for (i = 0; i < SIZE; i++)
      cmds[i] = NULL;
    for (i = 0; i < SIZE; i++)
      tokens[i] = NULL;
    
    i = 0, j = 0, cmdcount = 0, argcount = 0;
    // loop through each token in command string
    while(command != NULL) {
      cmdcount++;
      argcount--;
      // tokenize command arguments by whitespace delimiter
      token = strtok_r(command, " ", &saveptr);
      // add both command and its arguments to token array, each argument to args array, each command to cmds array
      cmds[cmdcount - 1] = token;
      while(token != NULL) {
        tokens[i] = token;
        if(j != 0) {
          args[argcount] = token;
        }
        i++;
        j++;
        argcount++;
        // traverse to next argument if exists
        token = strtok_r(NULL, " ", &saveptr);
      }
      // reset j counter to 0 and traverse to next command
      j = 0;
      command = strtok(NULL, "|");
    }
    // if exit command is entered, call exit to exit program
    if (strcmp(tokens[0], "exit") == 0) {
      exit(EXIT_SUCCESS);
    } 
    // if cd command is entered, call chdir to change directory
    else if (strncmp(tokens[0], "cd", strlen("cd")) == 0) {
      // cd only has 1 argument, so directory argument will be at index 0
      if (chdir(args[0]) < 0)
        perror("cd error");
    }
    // if history command is entered, call displayHistory to print history
    else if (strncmp(tokens[0], "history", strlen("history")) == 0) {
      // if argument is empty or is "-c", call executeHistory
      if (args[0] == NULL || strcmp(args[0], "-c") == 0) {
        // queues history, then executes it
        enqueue(history, &head, &tail, tmp);
        queued = 1;
        executeHistory(history, args[0], &head, &tail);
      }
      // if argument is a number, set execvpCondition to 1 and copy command at offset in history to input
      // while loop will reiterate without prompting and enter execvp portion of program
      else {
        execvpCondition = 1;
        // history index will be offset by head to account for movement of head in queue, it then queues the history with offset command
        strcpy(input, history[(atoi(args[0]) + head) % SIZE]);
        enqueue(history, &head, &tail, tmp);
        queued = 1;
      }
    }
    // if command(s) entered other than exit, cd, or history, execute command(s) using forks
    else {
      // if there is only one command
      if(cmdcount == 1) {
        // create child process using fork
        cpid = fork();
        // if fork fails, print error message and exit
        if(cpid == -1) {
          perror("fork");
          exit(EXIT_FAILURE);
        }
        // if child, execute command
        if(cpid == 0) {
          // creates args array based off how many arguments are in the command
          char* myargs[argcount+2];
          for(l = 0; l < argcount + 2; l++) {
            if(l == 0) { // if it is the first element, set it to the command
              myargs[l] = tokens[0];
            }
            else if(l == argcount + 1) { // if it is the last element, set it to null
              myargs[l] = NULL;
            }
            else { // set the middle elements to the arguments otherwise
              myargs[l] = args[l-1];
            }
          }
          execvp(myargs[0], myargs);
          // if execvp fails, print error message and exit
          perror("execvp failed in child 5");
          exit(EXIT_FAILURE);
        }
        // if parent, wait for child process to finish
        else {
          while(wait(NULL) >= 0);
        }
      }
      // if there are at least two commands communicating through pipe
      else {
        j = 0, k = 0, numargs = 0;
        // loops the same amount times as there are commands
        for(int i = 0; i < cmdcount; i++) {
          numargs = 0;
          // used to help calculate indices for which commands has what arguments
          while (1) {
            if(j == argcount) {
              k++;
              break;
            }
            if(tokens[k+1] == args[j]) {
              k++;
              j++;
              numargs++;
            }
            else {
              k++;
              break;
            }
          }
          // if we're on the last command, there is no need to create another pipe
          if(i == cmdcount - 1) {
            // if there are an even number of commands, the last pipe being used uses pipefd1, it will be pipefd2 for an odd number of commands as the final pipe
            if(i % 2 == 1) {
              cpid = fork(); // fork for cmd
              if (cpid < 0) {
                // if fork fails, print error message and exit
                perror("fork cpid failed");
                exit(EXIT_FAILURE);
              }
              if(cpid == 0) {
                close(pipefd1[1]); // not writing to pipefd1
                // change input of the process to the read end of pipefd1
                if (dup2(pipefd1[0], STDIN_FILENO) == -1) {
                  // if dup2 fails, print error message and exit
                  perror("dup2 failed in child 6");
                  exit(EXIT_FAILURE);
                }
                close(pipefd1[0]); // read end is no longer required because dup2
                char* myargs[numargs+2]; // array will contain command, null, and number of arguments
                for(l = numargs+1; l >= 0; l--) {
                  // if first entry in the array, it is the cmd
                  if(l == 0) {
                    myargs[l] = cmds[k-j-1];
                  }
                  // if it is the end of the array, add null
                  else if(l == numargs + 1) {
                    myargs[l] = NULL;
                  }
                  // adds arguments if it is not the beginning or the end
                  else {
                    myargs[numargs-l+1] = args[j-l];
                  }
                }
                execvp(myargs[0], myargs);
                // if execvp fails, print error message and exit
                perror("execvp failed in child 7");
                exit(EXIT_FAILURE);
              }
              close(pipefd1[0]); // close read end of pipe in parent
              close(pipefd1[1]); // close write end of pipe in parent
              while(wait(NULL) >= 0);
            }
            else {
              cpid = fork(); // fork for child
              if (cpid < 0) {
                // if fork fails, print error message and exit
                perror("fork cpid failed");
                exit(EXIT_FAILURE);
              }
              if(cpid == 0) {
                close(pipefd2[1]); // not writing to pipefd2
                // change input of the child process to read from the pipe
                if (dup2(pipefd2[0], STDIN_FILENO) == -1) { 
                  // if dup2 fails, print error message and exit
                  perror("dup2 failed in child 8");
                  exit(EXIT_FAILURE);
                }
                close(pipefd2[0]); // read end is no longer required because dup2
                char* myargs[numargs+2]; // array will contain command, null, and number of arguments
                for(l = numargs+1; l >= 0; l--) {
                  // if first entry in the array, it is the cmd
                  if(l == 0) {
                    myargs[l] = cmds[k-j-1];
                  }
                  // if it is the end of the array, add null
                  else if(l == numargs + 1) {
                    myargs[l] = NULL;
                  }
                  // adds arguments if it not the beginning or the end
                  else {
                    myargs[numargs-l+1] = args[j-l];
                  }
                }
                execvp(myargs[0], myargs);
                // if execvp fails, print error message and exit
                perror("execvp failed in child 9");
                exit(EXIT_FAILURE);
              }
              close(pipefd2[0]); // close read end in parent
              close(pipefd2[1]); // close write end in parent
              // waits for child
              while(wait(NULL) >= 0);
            }
            break; // break from the loop
          }
          if(i % 2 == 0) { // if i is even, pipefd1 is used
            if (pipe(pipefd1) == -1) {
              // if pipe fails, print error message and exit
              perror("pipe failed");
              exit(EXIT_FAILURE);
            }
            cpid = fork(); // create child process for cmd
            if (cpid < 0) {
              // if fork fails, print error message and exit
              perror("fork cpid failed");
              exit(EXIT_FAILURE);
            }
            // in child
            if (cpid == 0) {
              close(pipefd1[0]); // not reading from pipe
              // if it is not the first cmd, it will have pipefd2 in the process
              if(i != 0) {
                close(pipefd2[1]); // closes write end of pipe
                // makes input of child process read from pipefd2
                if (dup2(pipefd2[0], STDIN_FILENO) == -1) { 
                  // if dup2 fails, print error message and exit
                  perror("dup2 failed in child 10");
                  exit(EXIT_FAILURE);
                }
                close(pipefd2[0]); // close read end from pipefd2
              }
              // makes the output from child process write to pipefd1
              if (dup2(pipefd1[1], STDOUT_FILENO) == -1) { 
                // if dup2 fails, print error message and exit
                perror("dup2 failed in child 11");
                exit(EXIT_FAILURE);
              }
              close(pipefd1[1]); // write end is no longer required because dup2
              char* myargs[numargs+2]; // array will contain command, null, and number of arguments
              for(l = numargs+1; l >= 0; l--) {
                // if first entry in the array, it is the cmd
                if(l == 0) {
                  myargs[l] = cmds[k-j-1];
                }
                // if it is the end of the array, add null
                else if(l == numargs + 1) {
                  myargs[l] = NULL;
                }
                // adds arguments if it not the beginning or the end
                else {
                  myargs[numargs-l+1] = args[j-l];
                }
              }
              execvp(myargs[0], myargs);
              // if execvp fails, print error message and exit
              perror("execvp failed in child 1");
              exit(EXIT_FAILURE);
            }
            // if it isn't the first cmd, there will be a pipe using fd2 in the parent
            if(i != 0) {
              close(pipefd2[0]); // close read end from previous pipe in parent
              close(pipefd2[1]); // close write end from previous pipe in parent
            }
            // wait for child
            while(wait(NULL) >= 0);
          }
          else { // i is odd, pipefd2 is used
            if (pipe(pipefd2) == -1) {
              // if pipe fails, print error message and exit
              perror("pipe failed");
              exit(EXIT_FAILURE);
            }
            cpid = fork(); // fork for next process
            if (cpid < 0) {
              // if fork fails, print error message and exit
              perror("fork cpid failed");
              exit(EXIT_FAILURE);
            }
            if (cpid == 0) {
              close(pipefd2[0]); // not reading from pipefd2
              close(pipefd1[1]); // not writing to pipefd1
              // input of child process reads from pipefd1
              if (dup2(pipefd1[0], STDIN_FILENO) == -1) {
                // if dup2 fails, print error message and exit
                perror("dup2 failed in child 2");
                exit(EXIT_FAILURE);
              }
              close(pipefd1[0]); // read end is no longer required because dup2
              // output of child process writes to pipefd2
              if (dup2(pipefd2[1], STDOUT_FILENO) == -1) {
                // if dup2 fails, print error message and exit
                perror("dup2 failed in child 3");
                exit(EXIT_FAILURE);
              }
              close(pipefd2[1]); // write end is no longer required because dup2
              char* myargs[numargs+2]; // array will contain command, null, and number of arguments
              for(l = numargs+1; l >= 0; l--) {
                // if first entry in the array, it is the cmd
                if(l == 0) {
                  myargs[l] = cmds[k-j-1];
                }
                // if it is the end of the array, add null
                else if(l == numargs + 1) {
                  myargs[l] = NULL;
                }
                // adds arguments if it not the beginning or the end
                else {
                  myargs[numargs-l+1] = args[j-l];
                }
              }
              execvp(myargs[0], myargs);
              // if execvp fails, print error message and exit
              perror("execvp failed in child 4");
              exit(EXIT_FAILURE);
            }
              // close pipes for parent
              close(pipefd1[0]);
              close(pipefd1[1]);
              // wait for child to terminate
              while(wait(NULL) >= 0);
          }
         // wait for child to terminate
         while(wait(NULL) >= 0);
        }
      }
    }
    // checks if the command entered has been queued yet, this will only be used if history isn't the command called
    if(!queued) {
      enqueue(history, &head, &tail, tmp);
      queued = 1;
    }
    // frees the dynamically allocated space in tmp
    if(!execvpCondition) {
    free(tmp);
    }
  }
  return 0;
}