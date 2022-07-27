// Implement your shell in this source file.
#define _POSIX_SOURCE
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parse.h"

// declare runCommands since it is called in a variety of places
void runCommands(strarray_t *input, strarray_t *last);

// global flag to determine if shell should continue
static volatile int flag = 1;

/*
 * Signal handler for SIGINT.
 * Sets the global flag for runing the shell to 0.
 * This is done so we can first free memory before closing
 *
 */
void sigint_handler(int sig) {
  write(1, "mini-shell terminated\n", 22);
  // set flag to 0 to tell shell to exit while loop
  flag = 0;
}

/*
 * Function which represents the pipe case.
 * Function takes in a strarray_t of inputs and executes the first command
 * before the first pipe. It writes the output of the command to the pipe.
 * The function then executes the remaining commands after the first using
 * the read from pipe as an input.
 *
 * strarray_t *input - strarray_t of inputs to execute
 * strarray_t *last  - the last line of commands executed
 *                      exists for if prev is a comand in input
 * int loc   - index location of the first pipe in input
 */
void dopipe(strarray_t *input, strarray_t *last, int loc) {
  // split input into 2 strarray_t at the location of the first pipe
  strarray_t *first = (strarray_t *)malloc(sizeof(strarray_t));
  char **firstarr = (char **)malloc(sizeof(char *) * loc);
  for (int i = 0; i < loc; i++) {
    firstarr[i] =
        (char *)malloc(sizeof(char) * (strlen(input->strings[i]) + 1));
    strcpy(firstarr[i], input->strings[i]);
  }
  first->size = loc;
  first->strings = firstarr;

  strarray_t *second = (strarray_t *)malloc(sizeof(strarray_t));
  char **secondarr = (char **)malloc(sizeof(char *) * (input->size - loc - 1));
  for (int i = loc + 1; i < input->size; i++) {
    secondarr[i - loc - 1] =
        (char *)malloc(sizeof(char *) * (strlen(input->strings[i]) + 1));
    strcpy(secondarr[i - loc - 1], input->strings[i]);
  }

  second->size = input->size - loc - 1;
  second->strings = secondarr;
  freeStrArr(input);

  // init pipe
  int pipe_fds[2];
  assert(pipe(pipe_fds) == 0);
  int read_fd = pipe_fds[0];
  int write_fd = pipe_fds[1];

  // fork
  pid_t childID = fork();
  int childstatus;

  if (childID == 0) {
    close(read_fd);

    // write output of first command to pipe
    close(1);
    assert(dup(write_fd) == 1);
    runCommands(first, last);
  } else {
    // wait for child to finish
    waitpid(childID, &childstatus, 0);
    close(write_fd);

    // read from read end of pipe
    close(0);
    assert(dup(read_fd) == 0);

    // run commands with input from pipe
    runCommands(second, last);
  }
}

/*
 * The function that represents the case for semi-colon.
 * This function is very similar to pipe. Function takes in a strarray_t
 * of inputs and executes the first command which is before a semi-colon.
 * It then executes the commands after the semi-colon.
 *
 * strarray_t *input - the strarray_t of inputs to execute
 * strarray_t *last - the last line of commands sent. This exists
 *                    in the case that one of the commands in input is prev
 * int loc - index location fo the first semi-colon
 *
 *
 */
void dosemi(strarray_t *input, strarray_t *last, int loc) {
  // split input into 2 strarray_t at the first semi-colon
  strarray_t *first = (strarray_t *)malloc(sizeof(strarray_t));
  char **firstarr = (char **)malloc(sizeof(char *) * loc);
  for (int i = 0; i < loc; i++) {
    firstarr[i] =
        (char *)malloc(sizeof(char) * (strlen(input->strings[i]) + 1));
    strcpy(firstarr[i], input->strings[i]);
  }
  first->size = loc;
  first->strings = firstarr;

  strarray_t *second = (strarray_t *)malloc(sizeof(strarray_t));
  char **secondarr = (char **)malloc(sizeof(char *) * (input->size - loc - 1));
  for (int i = loc + 1; i < input->size; i++) {
    secondarr[i - loc - 1] =
        (char *)malloc(sizeof(char) * (strlen(input->strings[i]) + 1));

    strcpy(secondarr[i - loc - 1], input->strings[i]);
  }
  second->size = input->size - loc - 1;
  second->strings = secondarr;

  freeStrArr(input);

  // fork
  pid_t childID = fork();
  int child_status;
  if (childID == 0) {
    // execute first command first
    runCommands(first, last);
    exit(0);
  }
  // wait for child, run commands after semi-colon second
  waitpid(childID, &child_status, 0);
  runCommands(second, last);
}

/*
 * Represents the prev command.
 * Functions takes in strarray_t which represents the command line
 * previous to the current one being executed. Prints out this line
 * and then executes it.
 *
 * strarray_t *last - the last command line to be executed
 */

void prev(strarray_t *last) {
  // make sure there's something to exeute first
  if (last == NULL || last->size < 1) {
    // chose not to print anything since if you call prev on nothing,
    // you should get nothing back.
    return;
  }

  // print out the previous line of commands
  for (int i = 0; i < last->size; i++) {
    write(1, last->strings[i], strlen(last->strings[i]) + 1);
    write(1, " ", 1);
  }

  write(1, "\n", 1);

  // run previous line of commands
  runCommands(last, last);
  return;
}

/*
 * Reprents the help function.
 * If an arg after help is given, we print that
 * too many args were given. Otherwise, we print to the user
 * all built-in commands that exist in our shell.
 *
 * int size - the number of tokens given for the command
 */
void help(int size) {
  // make sure help is called by itself
  if (size > 1) {
    write(1, "Too Many Args\n", 14);
    return;
  }

  // message to print out
  char message[] = "This is Jonathan's Mini Shell\n"
                   "Built-in commands are:\n"
                   "cd - change directory\n"
                   "exit - terminate shell\n"
                   "source <file> - process each"
                   "line of a given file as cmds\n"
                   "prev - print out previous cmd and"
                   "execute it again\n"
                   "help - a list of all built-in commands\n";
  write(1, message, strlen(message) + 1);
  return;
}

/*
 * Represents the cd function.
 * Function takes in array of tokens and looks at the second token
 * and changes the directory specified in the second token. If there are not
 * exactly 2 tokens in the array, we print out that not enough or too many
 * args were given. If the directory does not exist, print that the directory
 * does not exist.
 *
 * char **input - the array of tokens which should have "cd" and some file
 * int size - the size of the input array
 */
void cd(char **input, int size) {

  // make sure we have proper number of args
  if (size > 2) {
    printf("Too many args\n");
    return;
  }

  // set arbitrary file path length to 256
  char curr[256];
  // get the current directory
  getcwd(curr, sizeof(curr));

  // if cd is called, just move up a level
  if (size == 1) {
    chdir("..");
    return;
  }

  // add the directory we want to move into to end of file path
  char *new = strcat(strcat(curr, "/"), input[1]);
  for (int i = 0; i < strlen(new); i++) {
    curr[i] = new[i];
  }

  // make sure we terminate the cstring just incase
  curr[strlen(new)] = '\0';

  // if chdir fails, print out a message
  if (chdir(input[1]) == -1) {
    printf("Directory: %s could not be entered\n", input[1]);
  }
}

/*
 * Represents the exit command.
 * Function finds the id of the parent process
 * and then sends a SIGINT signal to the parent process
 * which then exits the shell.
 */
void exit_shell() {
  // get ppid
  pid_t parent = getppid();
  // send signal to parent
  kill(parent, SIGINT);
  return;
}

/*
 * Represents the source command.
 * Function takes in an array of tokens which should be of size 2. If not then
 * print out a statement regarding incorrect argument size. Function opens the
 * file specified by the second token, if it does not exist then print invalid
 * and return. If everything has passed so far, read every line from the file
 * and execute the lines as separate commands.
 *
 * char **input - array of tokens, should be of size 2
 * int size  - size of array of tokens
 * strarray_t *last - the last commandline arg in the shell. Exists if any of
 * 		      the commands in the file contain prev.
 */
void source(char **input, int size, strarray_t *last) {
  // make sure we have proper number of args
  if (size < 2) {
    write(1, "Too Few Args\n", 13);
    return;
  }
  if (size > 2) {
    write(1, "Too Many Args\n", 14);
    return;
  }

  // open the file
  FILE *source;
  source = fopen(input[1], "r");
  // make sure opening the file is okay
  if (source == NULL) {
    write(1, "Invalid File\n", 14);
    return;
  }

  // buffer for line of commands
  char buffer[81];

  while (fgets(buffer, 81, source) != NULL) {
    // basically copy the main func
    buffer[strlen(buffer) - 1] = '\0';
    pid_t child = fork();
    int child_status;

    if (child == 0) {

      strarray_t *line = parse(buffer);
      runCommands(line, last);
    }

    waitpid(child, &child_status, 0);
  }
  return;
}

/*
 * A general function to run commands through the mini shell.
 * Functon takes in a strarray_t *input which is the current line of commands
 * and strarray_t *last which is the last line of commands. Function takes
 * input and copies the contents of input->strings onto a 81 arg array.
 * Normally it would be 16, but buffer can take up to 80 chars so I made it
 * 81 just so there isn't any overflow error anyways. Built-in commands
 * account for this, execvp will just error out if there there are too many
 * args If the array contains any pipes or semi-colons, delegates to dopipe
 * or dosemi. Otherwise check if myargv is one of our built in commands and
 * delegate to their specific functions, else execute the command using execvp.
 *
 *
 * strarray_t *input - tokens representing the current line of commands
 * strarray_t *last - tokens representing the last line of commands
 */
void runCommands(strarray_t *input, strarray_t *last) {
  char *myargv[81]; // set buffer size to 80. 81 for null char
  int lastArg = input->size;
  // place tokens in input into myargv
  for (int i = 0; i < input->size; i++) {
    myargv[i] = input->strings[i];
    myargv[i] = (char *)malloc(sizeof(char) * (strlen(input->strings[i]) + 1));
    strcpy(myargv[i], input->strings[i]);
    // if we encounter semi-colon or pipe, delegate
    if (strcmp(input->strings[i], ";") == 0) {
      dosemi(input, last, i);
      return;
    }
    if (strcmp(input->strings[i], "|") == 0) {
      dopipe(input, last, i);
      return;
    }
  }
  // free the input since we no longer use it
  freeStrArr(input);
  // check if the command is one of our built in commands
  if (strcmp(myargv[0], "prev") == 0) {
    prev(last);
    return;
  }

  if (strcmp(myargv[0], "help") == 0) {
    help(lastArg);
    return;
  }

  if (strcmp(myargv[0], "cd") == 0) {
    cd(myargv, lastArg);
    return;
  }

  if (strcmp(myargv[0], "exit") == 0) {
    exit_shell();
    return;
  }

  if (strcmp(myargv[0], "source") == 0) {
    source(myargv, lastArg, last);
    return;
  }

  // command wasn't built-in, so try to use exec on it
  // if that doesnt work, then we just say cmd couldnt execute
  else {
    myargv[lastArg] = NULL;

    if (execvp(myargv[0], myargv) == -1) {
      printf("Command: %s could not execute\n", myargv[0]);
    }
  }
}

/*
 * Main function for shell.
 * Initializes the shell and does IO operations.
 */

int main(int argc, char **argv) {
  // set buffer size to 80 chars max, 81 for null terminating
  char buffer[81];
  // mini-shell message printed each time
  char minishell[] = "mini-shell>";

  // init var for last cmd line input
  strarray_t *last;

  // check if we are on first loop, because we cant free
  // last if we never mallocd it
  int beginning = 0;

  // the current directory
  char curdir[256];
  getcwd(curdir, sizeof(curdir));

  // set our signal handler
  signal(SIGINT, sigint_handler);

  // while our global flag is 1, keep running
  while (flag) {
    // init a pipe
    // we need this to pipe info about the child directory
    int pipefds[2];
    pipe(pipefds);

    // print mini-shell> mesage
    write(1, minishell, 11);
    // take input from cmd line
    int length = read(0, buffer, 81);
    // terminate buffer
    buffer[length - 1] = '\0';
    // parse the buffer
    strarray_t *input = parse(buffer);

    // make sure the program is still supposed to be running
    // if not then free any allocated memory and break
    if (flag == 0) {
      freeStrArr(input);
      break;
    }

    // make sure there is a previous cmd to assign last to
    if (input->size > 0) {
      if (strcmp(input->strings[0], "prev") != 0) {
        // don't free if its the first loop
        // because no command stored
        if (beginning == 0) {
          beginning = 1;
        } else {
          freeStrArr(last);
        }

        // assign last a deep copy so we
        // can free input without any issue later
        last = duplicateStrArr(input);
      }
    }

    // fork
    pid_t childID = fork();
    int child_status;

    if (childID == 0) {
      // init var for child dir
      char childDir[257];
      close(pipefds[0]);
      // put a space in the pipe so if we don't call cd
      // the pipe doesn't stall
      write(pipefds[1], " ", 1);

      // run runCommands on input
      runCommands(input, last);

      getcwd(childDir, sizeof(childDir));
      write(pipefds[1], childDir, 257);
      exit(0);
    }
    // wait for child before continuing the loop
    waitpid(childID, &child_status, 0);

    // pipe child dir information to  parent
    close(pipefds[1]);
    // make sure we free input so it doesn't leak
    freeStrArr(input);

    // read from pipe
    // if the pipe has more than the space character
    // than we change the directory
    char l[256];
    read(pipefds[0], l, 256);
    if (strlen(l) > 1) {
      char newdir[256];
      // use memcpy to transfer data
      memcpy(newdir, &l[1], 256);

      chdir(newdir);
    }

    close(pipefds[0]);
  }

  freeStrArr(last);
  return 0;
}
