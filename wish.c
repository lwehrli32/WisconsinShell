#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

char *paths[100];
int num_paths = 0;
char error_message[30] = "An error has occurred\n";
int run_finished = 0;

void bashShell(char *args[]);
int searchPaths(char *cmd);

void checkMalloc(char *str){
    if (str == NULL){
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}

char *trim(char *str){
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}

void runCMD(char **args, char* fileName, int limit, int cmd){
	int pathsIndex = searchPaths(*(args));
	for(int t = 0; t < limit; t++){
		int rc = fork();
		
		if (rc == 0){

			if (cmd == 1){
				int i = 0;
				while(*(args + i) != NULL){
					if(strstr(*(args + i), "$") != NULL){
						char strInt[12];
						sprintf(strInt, "%d", t + 1);
						*(args + i) = strdup(strInt);
					}
				
					if (i == 254){
						break;
					}
					i++;
				}
			}

			if(pathsIndex > -1){
				char *path = paths[pathsIndex];
				if(strcmp(path, "/bin") != 0 && strcmp(path, "/usr/bin") != 0){
					char *temp = malloc(sizeof(char) * (strlen(*(args)) + strlen(paths[pathsIndex]) + 1));
					strcat(temp, paths[pathsIndex]);
					strcat(temp, "/");
					strcat(temp, *(args));
					strcpy(*(args), temp);
					free(temp);
					temp = NULL;
				}
			}
			
			if (fileName != NULL){
				(void) close(STDOUT_FILENO);
				open(fileName, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
			}
			
			execvp(args[0], args);
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}else if (rc > 0){
			(void)wait(NULL);
		}else{
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
	}
}

void cdCMD(int argsLen, char **args){
	if (argsLen != 3){
		write(STDERR_FILENO, error_message, strlen(error_message));
	}else{
		int rc = chdir(*(args + 1));
		if(rc != 0){
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
	}
}

int searchPaths(char *cmd){
	for (int i = 0; i < num_paths; i++){
	
		char *fullPath = malloc(sizeof(char) * (strlen(cmd) + strlen(*(paths + i)) + 1));
		checkMalloc(fullPath);
		strcpy(fullPath, strdup(""));
		strcat(fullPath, strdup(paths[i]));
		strcat(fullPath, strdup("/"));
		strcat(fullPath, strdup(cmd));
		
		int acc = access(fullPath, X_OK);
		free(fullPath);
		fullPath = NULL;	
		if (acc == 0){
			return i;
		}
	}
	return -1;
}

void addPath(int argslength, char **args){
	int number_of_paths = num_paths;
	if (argslength > 2){
		for(int i = 0; i < number_of_paths; i++){
			paths[i] = strdup("");
		}
		
		num_paths = 0;
		int buffer = 0;
		
		for (int i = 0; i < argslength; i++){
			if(args[i] != NULL){
				char *temp = (char *)malloc(sizeof(char) * (strlen(*(args + i))));
				checkMalloc(temp);
				strcat(temp, *(args + i));
				paths[i - buffer] = strdup(temp);
				num_paths++;
				free(temp);
			}else{
				buffer++;
			}
		}
	}else{
		for(int i = 0; i < number_of_paths; i++){
			paths[i] = strdup("");
		}
		num_paths = 0;
	}
}

void loopCMD(int argslength, char **args){

	if(argslength < 3){
		write(STDERR_FILENO, error_message, strlen(error_message));
		return;
	}
	
	// create new array without other args for loop
	char **newargs = (char **)malloc(sizeof(char) * argslength - 3);	
	if(newargs == NULL){
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	
	if(!isdigit(*(*(args + 1)))){
		write(STDERR_FILENO, error_message, strlen(error_message));
		return;
	}
	
	int loopLimit = atoi(*(args + 1));
	int loopArgsLength = 0;
		
	// put program args into an array with out program name
	for(int i = 2; i < argslength - 1; i++){
		*(newargs + i - 2) = (char *)malloc(sizeof(char) * strlen(*(args + i)));
		checkMalloc(*(newargs + i - 2));
		*(newargs + i - 2) = strdup(*(args + i));
		loopArgsLength++;
	}
	
	newargs[loopArgsLength] = NULL;
	int pathsIndex = -1;

	if(*(newargs) != NULL){
		pathsIndex = searchPaths(*(newargs));
	}else{
		int qq = 0;
		while (qq < loopArgsLength){
			if(*(newargs + qq) != NULL){
				pathsIndex = searchPaths(*(newargs + qq));
				if (pathsIndex > -1){
					break;
				}
				qq++;
			}
		}
	}
	
	// execute program or command
	if(pathsIndex != -1){
		char **cmdArgs = (char **)malloc(sizeof(char *) * loopArgsLength);
		if(cmdArgs == NULL){
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(0);
		}
		
		for (int i = 0; i < loopArgsLength; i++){
			if(i == 0){
				*(cmdArgs + i) = (char *)malloc(sizeof(char) * strlen(*(newargs + i)));
				checkMalloc(*(cmdArgs + i));
				*(cmdArgs + i) = strdup(*(newargs + i));
			}else{
				if(*(newargs + i) == NULL){
					*(cmdArgs + i) = *(newargs + i);
					break;
				}

				*(cmdArgs + i) = (char *)malloc(sizeof(char) * strlen(*(newargs + i)));
				checkMalloc(*(cmdArgs + i));
				*(cmdArgs + i) = strdup(*(newargs + i));
			}
		}
		cmdArgs[loopArgsLength] = NULL;
		
		runCMD(cmdArgs, NULL, loopLimit, 1);
			
		for(int i = 0; i < loopArgsLength; i++){
			*(cmdArgs + i) = strdup("");
		}
		free(cmdArgs);
	}else{
		if (strcmp(*(args), "cd") == 0){
			cdCMD(argslength, newargs);
		}else if(strcmp(*(args), "path") == 0){
			addPath(argslength, newargs);
		}
	}

	for(int i = 0; i < argslength; i++){
		*(newargs + i) = strdup("");
	}
	free(newargs);	
}

void interactiveCMDs(char *buffer){
    // no redirects
    char *found;
    char **args;
    int counter = 0;
    int cd = 0;
	int path = 0;
	int loop = 0;
    int pathsIndex = -1;
    	
	args = (char**)malloc(sizeof(char*) * 254);
	if(args == NULL){
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}

    while((found = strsep(&buffer, " ")) != NULL){
		if (counter == 0){
			pathsIndex = searchPaths(found);
            if (pathsIndex != -1){
				*(args + counter) = (char *)malloc(sizeof(char) * strlen(found));
				checkMalloc(*(args + counter));
				*(args + counter) = strdup(found);
            }else{
                if(strcmp(found, "cd") == 0){
                    cd = 1;
                }else if(strcmp(found, "path") == 0){
                    path = 1;
                }else if(strcmp(found, "loop") == 0){
                    loop = 1;
                }
            }
            
        }else{
			*(args + counter) = (char *)malloc(sizeof(char) * (strlen(found)));
			checkMalloc(*(args + counter));
			*(args + counter) = strdup(found);
        }
        counter++;
    }
   	args[counter] = NULL;
	counter++;
    
    if (pathsIndex != -1){
		runCMD(args, NULL, 1, 0);
    }else{
        if (cd){
            cdCMD(counter, args);
        }else if(path){
            addPath(counter, args);
        }else if(loop){
            loopCMD(counter, args);
		}else{
        	write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
	for(int i = 0; i <= counter; i++){
		*(args + i) = strdup("");
	}
	free(args);
}

void redirectCMDS(char *buffer){
	char *found;
	char **args;
	int counter = 0;
	int loop = 0;
	int	pathsIndex = -1;
	int foundCar = 0;
	char *fileName;
	int foundName = 0;

	args = (char**)malloc(sizeof(char*) * 254);
	if(args == NULL){
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	
	while((found = strsep(&buffer, " ")) != NULL){
		if (counter == 0){
			pathsIndex = searchPaths(found);
			if (pathsIndex != -1){
				*(args + counter) = (char *)malloc(sizeof(char) * strlen(found));
				checkMalloc(*(args + counter));
				*(args + counter) = strdup(found);
			}else{
				if(strcmp(found, "loop") == 0){
					loop = 1;
				}
			}

		}else if(strstr(found, ">") != NULL){
			// found redirection
			if(foundCar){
				foundCar = 0;
				break;
			}
			
			char *parts;
			while( (parts = strsep(&found,">")) != NULL ){
				if(strcmp(trim(parts), ">") != 0 && !foundCar){
					*(args + counter) = (char *)malloc(sizeof(char) * (strlen(parts)));
					checkMalloc(*(args + counter));
					*(args + counter) = strdup(parts);
					counter++;
					foundCar = 1;
				}else if(foundCar){
					foundName = 1;
	                fileName = (char *)malloc(sizeof(char) * (strlen(parts)));
		            checkMalloc(fileName);
			        fileName = strdup(parts);
				}
			}	
		}else if(foundCar){
			if(strlen(fileName) <= 1){
				// after redirection symbol
				foundName = 1;
				fileName = (char *)malloc(sizeof(char) * (strlen(found)));
				checkMalloc(fileName);
				fileName = strdup(found);
			}else{
				foundCar = 0;
				break;
			}
		}else if (!foundCar){
			*(args + counter) = (char *)malloc(sizeof(char) * (strlen(found)));
            checkMalloc(*(args + counter));
            *(args + counter) = strdup(found);
		}
		counter++;
	}

	if(foundCar && foundName){
		if(strlen(fileName) > 1){
			runCMD(args, fileName, 1, 1);
		}else{
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
	}else{
		write(STDERR_FILENO, error_message, strlen(error_message));
	}
	
	for(int i = 0; i <= counter; i++){
		*(args + i) = strdup("");
	}
	free(args);
}

void bashShell(char *args[]){
    FILE *file;
	file = fopen(args[1], "r");
	
    if (file == NULL){
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    char *line_buf = NULL;
    size_t line_buf_size = 0;
    int line_count = 0;
    ssize_t line_size;

    /* Get the first line of the file. */
    line_size = getline(&line_buf, &line_buf_size, file);
    line_buf[strcspn(line_buf, "\n")] = 0;
	strcpy(line_buf, trim(line_buf));

    /* Loop through until we are done with the file. */
    while (line_size >= 0 && strcmp(line_buf, "exit") != 0){

		if(strlen(line_buf) >  1){
			/* Increment our line count */
			line_count++;

			// loop while the user doesn't type 'exit'
			if (strstr(line_buf, ">") != NULL){
				// redirects
				redirectCMDS(line_buf);
			}else{
				// no redirects
				interactiveCMDs(line_buf);
			}
		}
        /* Get the next line */
        line_size = getline(&line_buf, &line_buf_size, file);
        line_buf[strcspn(line_buf, "\n")] = 0;
		strcpy(line_buf, trim(line_buf));
    }

    /* Free the allocated line buffer */
    free(line_buf);
    line_buf = NULL;
    fclose(file);
}

void interactiveShell(){
	char *buffer;
	size_t bufsize = 10;

	buffer = (char *)malloc(bufsize * sizeof(char));
	checkMalloc(buffer);

	printf("wish> ");
	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;
	strcpy(buffer, trim(buffer));

	// loop while the user doesn't type 'exit'
	while(strcmp(buffer, "exit") != 0){		
		if(strlen(buffer) > 1){
			if (strstr(buffer, ">") != NULL){
				// redirects
				redirectCMDS(buffer);
			}else{
				// no redirects
				interactiveCMDs(buffer);
			}	
		}

		printf("wish> ");
		getline(&buffer, &bufsize, stdin);
		buffer[strcspn(buffer, "\n")] = 0;
		strcpy(buffer, trim(buffer));
	}
	free(buffer);
}

int main(int argc, char *argv[]){
	paths[0] = "/bin";
	num_paths++;
	if(argc == 1){
		interactiveShell();
	}else if(argc == 2){
		bashShell(argv);
	}else{
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	exit(0);
}


