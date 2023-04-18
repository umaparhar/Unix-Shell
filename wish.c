#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int loopCount = 0;


struct Path{
	char *pathName;
	struct Path *next;
};

void addPath(char *pathName, struct Path *head){
	struct Path *cur = head;
	while(cur->next != NULL){
		cur = cur->next;
	}
	
	struct Path* newPath = NULL;
	newPath = (struct Path*)malloc(sizeof(struct Path));
	newPath->pathName = strdup(pathName);
	newPath->next = NULL;
	cur->next = newPath;
}

char* containsValidPath(struct Path *head, char *command){
	struct Path *cur = head->next;
	while(cur != NULL){
		
		char *fullPath = strcat(strdup(cur->pathName), "/");
		fullPath = strcat(fullPath,strdup(command));
		
		int valid = access(fullPath, X_OK);
		if(valid != -1){
			return fullPath;
		}
		cur = cur->next;
	}
	return NULL;
}

void cd(char *command, char *in){
	char *dir = strsep(&in, " ");
	char *ptr = strchr(dir, '\n');
	if(ptr){
		*ptr = '\0';
	}
	if(chdir(dir) != 0){
		
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message)); 
	}
}

void pathCommand(struct Path *head, char *command, char *in){
	head->next = NULL;
	char *pathName;
	while((pathName = strsep(&in, " ")) != NULL){
		
		char *ptr = strchr(pathName, '\n');
		if(ptr){
			*ptr = '\0';
		}				
		
		addPath(pathName, head);
	}
}


int allSpaces(char *check){
	while(*check != '\0'){
		if(!isspace((unsigned char) *check)){
			return 0;
		}
		check++;
	}
	return 1;
}

void regularPath(struct Path *head, char *command, char *in){
	char *output = NULL;
	if(strcmp(command, "$loop") == 0){
		sprintf(command, "%d", loopCount);
		loopCount++;
	}	
	char *ptr = strchr(command, '\n');
	if(ptr){
		*ptr = '\0';
	}
	
	char *path = containsValidPath(head, command);
	
	if(path != NULL){
		char *ptr = strchr(path, '\n');
		if(ptr){
			*ptr = '\0';
		}
		char *args [10];
		args[0] = command;
		int i = 1;
		int error = 0;
		while((command = strsep(&in, " ")) != NULL){
			if(allSpaces(command)){
				break;
			}
			char *ptr = strchr(command, '\n');
			if(ptr){
				*ptr = '\0';
			}
			if(strcmp(command, "$loop") == 0){
				sprintf(command,"%d", loopCount);
				loopCount++;
			}
			
			if(strcmp(command, ">") == 0){
				output = strsep(&in, " ");
				char *next = strsep(&in, " ");
				if(output == NULL || next != NULL){
					error = 1;
					char error_message[30] = "An error has occurred\n";
    					write(STDERR_FILENO, error_message, strlen(error_message));
					break;
				}
				char *ptr = strchr(output, '\n');
				if(ptr){
					*ptr = '\0';
				}
				break;	
			}
			else if(strstr(command, ">") != NULL){
				char * arg = strsep(&command, ">");
				args[i] = arg;
				output = strsep(&command, ">");
				i++;
				break;
			}
			else{	
				args[i] = command;
				i++;
			}
			//i++;
		}
		if(error == 0){
		args[i] = NULL;
		int out = -1;
		int pid2 = fork();
		if(pid2 == 0){
			if(output != NULL){
				mode_t mode = 0777;
				out = open(output, O_CREAT | O_WRONLY, mode);
			}
			if(execv(path, args) < 0){
				char error_message[30] = "An error has occurred\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
			}

			
		}
		else{
			(void)wait(NULL);
			if(out != -1){
				close(out);
			}
		}
		}
	}
	else{
		char error_message[30] = "An error has occurred\n";
    		write(STDERR_FILENO, error_message, strlen(error_message));
	}
}

void loop(struct Path *head, char *command, char *in){
	loopCount = 1;
	command = strsep(&in, " ");
	int numLoops = atoi(command);
	if(numLoops <= 0){
		char error_message[30] = "An error has occurred\n";
    		write(STDERR_FILENO, error_message, strlen(error_message));
	}
	else{
		command = strsep(&in, " ");
		
		for(int i = 0; i < numLoops; i++){
			char *copyNew = strdup(in);
			char *copyCom = strdup(command);
			if(strcmp(copyCom, "$loop") == 0){
				sprintf(copyCom, "%d", loopCount);
				loopCount++;
			}	
			regularPath(head, copyCom, copyNew);	
		}
	}
}

//source: https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
void trimLeadingWhiteSpace(char *line){
	int index, i;
	index = 0;
	while(line[index] == ' '){
		index++;
	}
	if(index != 0){
		i = 0;
		while(line[i + index] != '\0'){
			line[i] = line[i + index];
			i++;
		}
		line[i] = '\0';
	}
}

int main(int argc, char *argv[]){
	
	struct Path* head = NULL;
	head = (struct Path*)malloc(sizeof(struct Path));
	head->pathName = "head";
	head->next = NULL;
	addPath("/bin", head);
 
	char input[256];
	char *in = input;
	size_t size = 256;
	char *command;

	loopCount = 1;

	if(argc > 2){
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	else if(argc == 2){
		FILE *file = fopen(argv[1], "r");
		
		if(file == NULL){	
			char error_message[30] = "An error has occurred\n";
    			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
		else{
			char *line = NULL;
			size_t len = 0;
			ssize_t read;
			
			while((read = getline(&line, &len, file)) != -1){
				if(allSpaces(line)){
					continue;
				}
				trimLeadingWhiteSpace(line);
				command = strsep(&line, " ");
				
				loopCount = 1;
					
				if(strcmp(command, "exit\n") == 0){
					exit(0);
				}
				
				else if(strcmp(command, "loop") == 0){
					loop(head, command, line);
				}
				else if(strcmp(command, "cd") == 0){
					cd(command, line);
				}
				else if(strcmp(command, "path\n") == 0 || strcmp(command, "path") == 0){
					pathCommand(head, command, line);
				}	
				
				else if(strcmp(command, ">") == 0){
					char error_message[30] = "An error has occurred\n";
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
				else if(strcmp(command, "") == 0){
					continue;
				}
				
				else if(strcmp(command, " ") == 0){
					continue;
				}
				else{
					regularPath(head, command, line);
				}
				
			}
			if(command ==  NULL){			
				char error_message[30] = "An error has occurred\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			//fclose(file);
		}
		fclose(file);
	}
	else{
		while(strcmp(input, "exit\n") != 0){
			printf("wish>");
			getline(&in, &size, stdin);
			if(in == NULL){
				continue;
			}
			trimLeadingWhiteSpace(in);
			command = strsep(&in, " ");
			loopCount = 1;
			if(strcmp(command, "exit\n") == 0){
				exit(0);
			}
			else if(strcmp(command, "loop") == 0){
				loop(head, command, in);
			}
			else if(strcmp(command, "cd") == 0){
				cd(command, in);
			}
			else if(strcmp(command, "path") == 0 || strcmp(command, "path\n")== 0){
				pathCommand(head, command, in);
			}	
				
			else if(strcmp(command, "") == 0){
				continue;
			}
			
			else if(strcmp(command, " ") == 0){
				continue;
			}
			else if(strcmp(command, ">") == 0){
				char error_message[30] = "An error has occurred\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			else{
				regularPath(head, command, in);
			}
		}
	
	struct Path* current = head;
	while(current != NULL){
		free(current);
		current = current->next;
	}
	}

}
