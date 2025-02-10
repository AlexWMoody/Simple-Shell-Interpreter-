#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Function to get the appropriate values to be displayed in the prompt
void get_prompt(){
	char host[256];
	char *login;
	char path[256];

	gethostname(host, sizeof(host));
	login = getlogin();
	getcwd(path, sizeof(path));

	strcat(host, ": ");
	strcat(login, "@");
	strcat(path, " >");
	strcat(login, host);
	strcat(login, path);

	const char* prompt = login;
}

// Structure to hold information about a background job
struct BackgroundJob {
    pid_t pid;
    char *command;
	char *arguments;
};

// Structure of a node for linked list to contain background jobs
struct node {
   struct BackgroundJob *job;
   struct node *next;
};
struct node *head = NULL;
struct node *current = NULL;

int num_background_jobs = 0;

// Function to add a background job to the linked list
void add_job(pid_t pid, char *command, char *arguments) {
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    struct BackgroundJob* new_job = (struct BackgroundJob*)malloc(sizeof(struct BackgroundJob));
    new_job->arguments = strdup(arguments);
    new_job->command = strdup(command);
    new_job->pid = pid;
    new_node->job = new_job;
    new_node->next = NULL;

    if(head == NULL){
        head = new_node;
    }else{
        current = head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_node;
    }
    num_background_jobs++;
}

// Function to display the list of background jobs
void list_jobs() {
	if(head != NULL){
		struct node *temp = head;
		while(temp->next != NULL){
			printf("%d: %s %s\n", temp->job->pid, temp->job->command, temp->job->arguments);
			temp = temp->next;
		}
		printf("%d: %s %s\n", temp->job->pid, temp->job->command, temp->job->arguments);
	}
		printf("Total Background jobs: %d\n", num_background_jobs);
}

// Function to check if and wait for background jobs termination
// The function notifies the user of the job termination and removes the job from the list
void check_jobs() {
    if(num_background_jobs > 0){
		int status;
		pid_t ter;
		while ((ter = waitpid(0, NULL, WNOHANG)) > 0) {
            struct node *temp = head;
            while(temp != NULL){
				if (temp->job->pid == ter) {
					printf("%d: %s %s has terminated.\n", ter, temp->job->command, temp->job->arguments);
                    struct node *temp2;
                    if(head->job == temp->job){
                        temp2 = head;
                        head = head->next;
                        free(temp2);
                    }else {
                        struct node *cur = head;
                        while(cur->next != NULL){
                            if(cur->next->job == temp->job){
                                temp2 = cur->next;
                                cur->next = cur->next->next;
                                free(temp2);
                                break;
                            }else{
                                cur = cur->next;
                            }
                        }
                    }
					num_background_jobs--;
					break;
				}
				temp = temp->next;
            }
		}
	}
}

int
main(int argc, char* argv[])
{
	//Get prompt the first time before entering loop
	char host[256];
	char *login;
	char path[256];

	gethostname(host, sizeof(host));
	login = getlogin();
	getcwd(path, sizeof(path));

	strcat(host, ": ");
	strcat(login, "@");
	strcat(path, " >");
	strcat(login, host);
	strcat(login, path);

	const char* prompt = login;

	int bailout = 0;
	while (!bailout) {
		//Get prompt and display prompt while using readline
		get_prompt();
		char* reply = readline(prompt);

		//Checks if the user enters no input (ie. presses enter to go to a new line)
		//Skips iteration of loop if this is the case
		if(!strcmp(reply, "")){
			continue;
		}

		//Check if the user types "exit\n" to exit the SSI
		if (!strcmp(reply, "exit")) {
			bailout = 1;
		}

		//Tokenize the user's input
		char *tokens[1024];
		int token_count = 0;
		char *token = strtok(reply, " ");
		while (token != NULL) {
			tokens[token_count++] = token;
			token = strtok(NULL, " ");
		}
		tokens[token_count] = NULL;

		//check for cd command
		if(strcmp(tokens[0], "cd") == 0){
			char *dir;
			if(token_count < 2){
				dir = getenv("HOME");
			}else if(strcmp(tokens[1], "~") == 0){
				dir = getenv("HOME");
			}else{
				dir = tokens[1];
			}
			if (dir == NULL){
                printf("Error: HOME environment variable not set.\n");
            }else if (chdir(dir) != 0){
                perror("cd failed");
            }
		//check for bg command
		}else if(strcmp(tokens[0], "bg") == 0){
			if (token_count > 1) {
                pid_t child_pid = fork();

                if (child_pid == -1) {
                    perror("Fork failed");
                    exit(1);
                }

                if (child_pid == 0) {
                    //This is the child process
                    if (execvp(tokens[1], tokens + 1) == -1) {
                        printf("Execution of command %s failed\n", tokens[1]);
						perror("Execution failed");
                        exit(1);
                    }
					continue;
                } else {
                    //This is the parent process
					char temp[1024]= "";
					for(int i = 2; i < token_count; i++){
						strcat(temp, tokens[i]);
						if(i < token_count - 1){
							strcat(temp, " ");
						}
					}
                    add_job(child_pid, tokens[1], temp);
                }
            } else {
                printf("Usage: bg <command>\n");
            }
		//check for bglist command
		}else if(strcmp(tokens[0], "bglist") == 0){
			list_jobs();
		}else{
			//Fork a new process
			pid_t child_pid = fork();

			if (child_pid == -1) {
				perror("Fork failed");
				exit(1);
			}

			if (child_pid == 0) {
				//Newly spawned child Process.
				int status_code = execvp(tokens[0], tokens);
				if (status_code == -1) {
					if(strcmp(reply, "exit")){
						printf("Error with command: %s\n", tokens[0]);
					}
					return 1;
				}
			}else {
				//Old Parent process. The C program will come here
				int status;
				waitpid(child_pid, &status, 0);
			}
		}
		
		free(reply);
		
		//Check for terminated background jobs
		check_jobs();
	}
	//Message for the user as they exit the SSI
	printf("Bye Bye\n");
}
