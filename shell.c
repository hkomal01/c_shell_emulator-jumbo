/* 
 * Harshdeep Singh Komal (hkomal01)
 *
 * P1: Shell
*/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "unistd.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

/*
 * execute_piped_commands
 *
 * Arguments: An array of array of strings contaning every command as their 
 * first element and each commands' arguments as the rest of the elements
 * and also takes in an integer containing the number of commands.
 *
 * Returns: Nothing
 *
 * Note: Goes through and make sure every command is executed and piped properly
 * with error checking/printing also done.
 */
void execute_piped_commands(int num_commands, char*** commands) {
    int pipes[2 * (num_commands - 1)];

    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes + i * 2) < 0) {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            if (i > 0) {
                if (dup2(pipes[(i - 1) * 2], 0) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            if (i < num_commands - 1) {
                if (dup2(pipes[i * 2 + 1], 1) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            // Redirect stderr to stdout
            if (dup2(1, 2) < 0) {
                perror("dup2");
                exit(1);
            }

            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipes[j]);
            }

            execvp(commands[i][0], commands[i]);

            if (errno == ENOENT) {
                fprintf(stderr, "jsh error: Command not found: %s\n", commands[i][0]);
                exit(127);
            } else {
				printf("yo\n");
                perror(commands[i][0]);
                exit(127);
            }
            exit(127);
        } else if (pid < 0) {
            perror("fork");
            exit(1);
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipes[i]);
    }

    int status;
    for (int i = 0; i < num_commands; i++) {
        wait(&status);
    }

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        printf("jsh status: %d\n", exit_status);
    }
}

/*
 * change_dir
 *
 * Arguments: An array of array of strings contaning every command as their 
 * first element and each commands' arguments as the rest of the elements
 * and also takes in an integer containing the number of commands.
 *
 * Returns: Nothing
 *
 * Note: If any command is cd, change_dir only runs cd because cd doesn't
 * support piping. If there is an error we print status 127 and 0 otherwise.
 * Otherwise, we change the directory to the one specified or home if there 
 * isn't one specified.
 */
void change_dir(int num_commands, char*** commands) {
	//Int to hold index of cd command
	int i;
	//Return status
	int status = 0;
	//Find cd command's index
	for (i = 0; i < num_commands; i++) {
		if (!(strcmp(commands[i][0], "cd"))){
			break;
		}
	}

	//Change directories
	if (commands[i][1] == NULL) {
		char* home = getenv("HOME");
		//If no home
		if (home == NULL) {
			status = 127;
			printf("Error: No home\n");
			printf("jsh status: %d\n", status);
			return;
		}
		//Change directory to home and print potential error
		if (chdir(home) != 0) {
			perror("cd");
			status = 127;
		}
	} else { //If not home directory
		//Change directory and print potential error
		if (chdir(commands[i][1]) != 0) {
			perror("cd");
			status = 127;
		}
	}
	//Print our status (0 for success, 127 otherwise)
	printf("jsh status: %d\n", status);
}

/*
 * substrings
 *
 * Arguments: A string to find the substrings seperated by spaces of
 *
 * Returns: An array containing every substring seperated by a space.
 *
 * Note: none
 */
char** substrings(const char* str) {
    int spaces = 0;
    int len = strlen(str);

	//Count number of substrings that will be needed = spaces (+ 1)
    for (int i = 0; i < len; i++) {
        if (isspace(str[i])) {
            spaces++;
        }
    }

	//Allocate space for all arguments and NULL
    char** substrings = malloc((spaces + 2) * sizeof(char**));
	//If malloc fails, return NULL
	if (substrings == NULL) {
        fprintf(stderr, "Error: malloc failed in substrings\n");
        return NULL;
    }
	//Initialize values all to NULL
	for (int i = 0; i < spaces + 2; i++) {
		substrings[i] = NULL;
	}
    int substring_start = 0;
    int substring_end = 0;
    int substring_index = 0;

    while (substring_end <= len) {
		//If a space is found or we've reached the end of the string
        if (isspace(str[substring_end]) || substring_end == len) {
            int substring_len = substring_end - substring_start;

			//Allocate space for substring and NULL
            substrings[substring_index] = malloc(substring_len + 1);
			//If malloc fails we free al successful malloc calls and return NULL
			if (substrings[substring_index] == NULL) {
                fprintf(stderr, "Error: malloc failed in substrings\n");
                for (int j = 0; j < substring_index; j++) {
                    free(substrings[j]);
                }
                free(substrings);
                return NULL;
            }

			//Use (safe) strncpy to copy our newly found substring to array
            strncpy(substrings[substring_index], str + substring_start, 
												 substring_len);

			//Add a NULL character
            substrings[substring_index][substring_len] = '\0';
            
			//Look for new substring
			substring_index++;
            substring_start = substring_end + 1;
        }

		//Increment end to determine length of substring when nonspace 
		//character is seen
        substring_end++;
    }

    return substrings;
}

/*
 * trimSpaces
 *
 * Arguments: A string to find the first/last nonspace character of and an
 * array of integers to store them in.
 *
 * Returns: Nothing
 *
 * Note: none
 */
void indices(char* argument, int* indices)
{
	int idx = 0;
	indices[0] = 0;
	indices[1] = strlen(argument);
	//Find first nonspace char
	while (isspace(argument[idx])) {
		if (argument[++idx] == 0) {
			return;
		}
	}
	indices[0] = idx;
	
	//Find last nonspace char
	idx = strlen(argument);
	while (isspace(argument[idx])) {
		if (--idx == -1) {
			return;
		}
	}
	indices[1] = idx;
}

/*
 * trimSpaces
 *
 * Arguments: A string to trim the spaces of and an array containing the 
 * indices which contain the first and last nonspace character of the string
 *
 * Returns: Nothing
 *
 * Note: This helper function reduces any sequences of spaces greater than 1
 * to just one space and moves everything over to accomodate that.
 */
void trimSpaces(char *str, int* idxs) {
    int i, j;
    int len = idxs[1];
    int space_count = 0;

    // double check trailing spaces
    while (len > idxs[0] && isspace(str[len - 1])) {
        len--;
    }

	//For every space we remove them and replace them with nonspace chars
    for (i = idxs[0], j = idxs[0]; i < len; i++) {
        if (isspace(str[i])) {
            space_count++;
        } else {
            space_count = 0;
        }
        if (space_count <= 1) {
            str[j++] = str[i];
        }
    }

	//New ending point is J
    str[j] = '\0';
    idxs[1] = j;
}

/*
 * executor
 *
 * Arguments: A string arr containing each command as entered by the user 
 * and an integer holding the length of that array
 *
 * Returns: Nothing
 *
 * Note: This function takes every command as entered by the user and first
 * tries to 'clean' it up by removing any extraneous spaces or tabs. It then
 * takes those 'cleaned' strings and maps each command to an array of strings
 * which contains the command and all of its arguments as elements. This array
 * of strings is stored in another array. Finally, it calls 
 * execute_piped_commands with the newly populated array and the number of 
 * commands.
 */
void executor(char** args, int commands) 
{
	//Allocate memory for working with current commands
	char** current = malloc(commands * sizeof(char**));
	//Allocate memory for array of array of strings which will store each
	//Command as an array of itself and its arguments
	char*** toExec = malloc(commands * sizeof(char***));
	//Array to count the number of arguments for each command
	int* numArgs = malloc(commands * sizeof(int));
	//Flag that checks if user is trying to change directories
	bool cd = false;
	//If any malloc failure, free any successful mallocs and return
	if (current == NULL) {
		fprintf(stderr, "Error: malloc failed\n");
		return;
	}
	if (toExec == NULL) {
		fprintf(stderr, "Error: malloc failed\n");
		free(current);
		return;
	}
	if (numArgs == NULL) {
		fprintf(stderr, "Error: malloc failed\n");
		free(current);
		free(toExec);
		return;
	}

	//Loop over each command to create toExec
	for (int i = 0; i < commands; i++) {
		//Variable to count spaces and an array to keep track of
		//indices
		int spaces = 0;
		int* idxs = malloc(2 * sizeof(int));
		//If malloc fails free all successful mallocs and return
		if (idxs == NULL) {
			fprintf(stderr, "Error: malloc failed\n");
			for (int j = 0; j < i; j++) {
				free(current[j]);
				free(toExec[j]);
			}
			free(current);
			free(toExec);
			free(numArgs);
			return;
		}

		//Get the indices of first and last nonspace characters
		indices(args[i], idxs);
		//Trim and realign the nonspace characters to remove consecutive spaces
		trimSpaces(args[i], idxs);
		//True length of the command with trimmed spaces
		int newLen = idxs[1] - idxs[0] + 1; // add 1 for null char

		//Count number of arguments = count number of spaces (+ 1)
		for (int j = idxs[0]; j < idxs[1] - 1; j++) {
			if (isspace(args[i][j])) {
				spaces++;
			}
		} 

		current[i] = malloc(newLen);
		//If malloc fails free all successful mallocs and return
		if (current[i] == NULL) {
			fprintf(stderr, "Error: malloc failed\n");
			for (int j = 0; j < i; j++) {
				free(current[j]);
				free(toExec[j]);
			}
			free(current);
			free(toExec);
			free(numArgs);
			free(idxs);
			return;
		}

		//Put our commands into a new array so they are 0-indexed.
		for (int j = idxs[0]; j < idxs[1]; j++) {
			current[i][j - idxs[0]] = args[i][j];
		}
		current[i][newLen - 1] = '\0'; //End with newline character

		//Store all substrings seperated by a space in an array of strings
		char** substring = substrings(current[i]);
		//If malloc fails in substrings free all successful mallocs and return
		if (substring == NULL) {
			fprintf(stderr, "Error: malloc failed in substring\n");
			for (int j = 0; j < i; j++) {
				free(current[j]);
				free(toExec[j]);
			}
			free(current[i]);
			free(current);
			free(toExec);
			free(numArgs);
			free(idxs);
			return;
		}

		//Store substrings in toExec[i] and set their number of arguments
		toExec[i] = substring;
		numArgs[i] = spaces + 1;

		//Free temporary memory
		free(idxs);
		free(current[i]);
	}

	for (int i = 0; i < commands; i++) {
		if (!(strcmp(toExec[i][0], "cd"))) {
			cd = true;
		}
	}
	
	//Handle special case of cd
	if (cd) change_dir(commands, toExec);
	else execute_piped_commands(commands, toExec);

	//Free all memory used
	for (int i = 0; i < commands; i++) {
		for (int j = 0; j < numArgs[i]; j++) {
			free(toExec[i][j]);
		}
		free(toExec[i]);
	}
	free(current);
	free(numArgs);
	free(toExec);
}

/*
 * readProcess
 *
 * Arguments: A string containing user input
 *
 * Returns: int 0, if successful and anything else if not
 *
 * Note: This function takes user input and simply separates all commands 
 * inputted by the user that were seperated by the pipe operator. It then
 * calls executor() with a char** holding an array of strings containing 
 * each command as well as with the number of commands
 */
int readProcess(char* input)
{
	char** args = NULL; //Store commands
	int len = strlen(input); 
	//Create a buffer of same length and copy input to it
	char command[len+1]; 
	char* ptr = strcpy(command, input);
	int numComms = 0; //number of commands initialized

	//Count number of commands
	ptr = strtok(command, "|");
	while (ptr != NULL) {
		numComms++;
		ptr = strtok(NULL, "|");
	}

	//Allocate to store every command as a string
	args = malloc(numComms * sizeof(char*));
	if (args == NULL) {
		fprintf(stderr, "Error: malloc failed\n");
		return 1;
	}
	//Get every command to the pipe and store it in args
	ptr = strtok(input, "|");
	args[0] = ptr;
	for (int i = 1; i < numComms; i++) {
		ptr = strtok(NULL, "|");
		args[i] = ptr;
	}

	//Send array of commands as strings with their amount to executor
	executor(args, numComms);

	free(args);
	
	return 0;
}

int main(int argc, char* argv[])
{
	//Hold input and its length
	char *input = NULL;
	size_t len = 0;
	int read = 0;

	while (1) {
		printf("$jsh ");
		read = getline(&input, &len, stdin);
		//Check for error or if input has ended w/o "exit"
		if (read == -1) {
			fprintf(stderr, "Error: getline failed\n");
		} else if (!read) {
			free(input);
			return 0;
		} else { //Grab input to newline and if not exit, process it
			input[strcspn(input, "\n")] = 0;
			if (!strcmp(input, "exit")) {
				free(input);
				return 0;
			} //else
			readProcess(input);
			wait(NULL);
		}
	}
	
	return 0;
}
