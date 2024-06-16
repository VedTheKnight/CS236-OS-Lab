#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Error handling
// Kill message for all processes on exit

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_BACKGROUND_PROCESSES 4

char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){ //multiple spaces are ignored
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
		tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
		strcpy(tokens[tokenNo++], token);
		tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

int getCommandLength(char** array) {
    int length = 0;

    while (array[length] != NULL) {
        length++;
    }

    return length;
}

void reap_background_processes(int bg_procs[], int *bg_proc){
	int status;
	int pid;

	while((pid = waitpid(-1,&status,WNOHANG)) > 0){ //WNOHANG is non blocking which means the executing can reap_background_processes(bg_procs,&bg_proc);continue. \
		//	-1 denotes any background process

		//remove the pid from the process table and update its index
		for(int i = 0; i<*bg_proc; i++){
			if(bg_procs[i] == pid){
				for(int j = i+1; j<*bg_proc;j++){
					bg_procs[j-1] = bg_procs[j];
				}
			}
		}
	
		*bg_proc = *bg_proc-1;
		bg_procs[*bg_proc] = 0;

		if(WIFEXITED(status)){
			printf("Shell: Background process finished (pid: %d)\n",pid);
		}
		else{
			printf("Shell: Background process was killed (pid: %d)\n",pid);
		}
	}
}

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int bg_procs[MAX_BACKGROUND_PROCESSES] = {0}; //maximum number of background processes initialized to dummy
	int bg_proc = 0;
	int i;
	
	signal(SIGINT, SIG_IGN);

	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();


		line[strlen(line)] = '\n'; //terminate with new line

		if(strcmp(line,"\n")==0){ //this ensures that a blank command doesn't cause a segmentation fault. Otherwise it would segfault at the check for cd
			reap_background_processes(bg_procs,&bg_proc);
			continue;
		}

		//here we reap the 
		tokens = tokenize(line);

		if(strcmp(tokens[getCommandLength(tokens)-1],"&") == 0){ //background process implementation

			//update the background process table
			if(bg_proc == MAX_BACKGROUND_PROCESSES){
				printf("Shell: [ERROR] Too many background processes running \n");
				reap_background_processes(bg_procs,&bg_proc);
				continue;
			}
			else{
				int pid = fork();

				if(pid == 0){
					
					daemon(0,0);
					char *myargs[MAX_NUM_TOKENS * sizeof(char *)]; //similar to tokens

					for(i=0; strcmp(tokens[i],"&") != 0; i++){
						myargs[i] = tokens[i];
					}

					if(execvp(myargs[0],myargs)!=0){
						printf("Shell: [Error] Invalid Command\n");
					}

					exit(1);
				}else{ //parent process reaps the children

					bg_procs[bg_proc++] = pid; // only here is the value of pid determined as the child's pid
					printf("Shell: Background process started (pid: %d)\n",pid);
					reap_background_processes(bg_procs,&bg_proc);
					continue; //continues to the next iteration of the loop as though nothing happened
				}
			}
			continue;
		}

		int ser_sep_index[64] = {0};
		ser_sep_index[0] = -1;
		int par_sep_index[64] = {0};
		par_sep_index[0] = -1;

		int ctr_ser = 1; //we will use each && to denote the start of a command
		for(int i = 0; i<getCommandLength(tokens); i++){
			if(strcmp(tokens[i],"&&") == 0){
				ser_sep_index[ctr_ser++] = i;
			}
		}

		int ctr_par = 1; //we will use each &&& to denote the start of a command
		for(int i = 0; i<getCommandLength(tokens); i++){
			if(strcmp(tokens[i],"&&&") == 0){
				par_sep_index[ctr_par++] = i;
			}
		}

		if(ctr_ser > 1 && ctr_par > 1){ // checks command validity
			//check for any zombie processes and free them
			printf("Shell: [Error] Can't run series and parallel together\n");
			reap_background_processes(bg_procs,&bg_proc);

			// Freeing the allocated memory	
			for(i=0;tokens[i]!=NULL;i++){
				free(tokens[i]);
			}
			free(tokens);

			continue;
		}

		ser_sep_index[ctr_ser] = getCommandLength(tokens);

		//series execution logic 
		if(ctr_ser > 1){
			for(int i = 0; i<ctr_ser;i++){
				//debugging command :
				if(strcmp(tokens[ser_sep_index[i]+1],"debug") == 0){
					printf("Process Table Size : %d\n",bg_proc);
					printf("Running background processes : \n");
					for(int j = 0; j<MAX_BACKGROUND_PROCESSES; j++){
						printf("%d\n",bg_procs[j]);
					}
					reap_background_processes(bg_procs,&bg_proc);
					continue;
				}

				if(strcmp(tokens[ser_sep_index[i]+1],"cd") == 0){

					if(strcmp(tokens[ser_sep_index[i]+3],"&&") != 0){
						printf("Shell: [Error] Too many arguments for cd\n");
						reap_background_processes(bg_procs,&bg_proc);
						continue;
					}

					if (chdir(tokens[ser_sep_index[i]+2]) != 0) {
						printf("Shell: [Error] Invalid directory\n");
					}

					reap_background_processes(bg_procs,&bg_proc);
					continue;
				}

				if(strcmp(tokens[ser_sep_index[i]+1], "exit") == 0){ 

					printf("Shell: Killing all background processes...\n");

					int ppid = getpid();

					for(int j=0; j<bg_proc; j++){
						kill(bg_procs[j], SIGKILL); //sends a SIGKILL signal to specific pid of bg process
						// One important fact is that SIGTERM terminates the process and allows some cleanup. SIGKILL would just kill it
					}
					
					reap_background_processes(bg_procs,&bg_proc); //Clears all the zombie processes

					printf("Shell: Freeing all dynamically allocated memory...\n");
					
					//free dynamically allocated memory
					for(int j=0;tokens[j]!=NULL;j++){
						free(tokens[j]);
					}
					free(tokens);

					printf("Shell: Exited successfully\n");
					exit(0);
				}

				int pid = fork(); //this return's the child's PID

				if(pid == 0){
					// printf("I am child. My PID is %d.\n",getpid());
					signal(SIGINT, SIG_DFL);

					char *myargs[MAX_NUM_TOKENS * sizeof(char *)]; //similar to tokens

					int x = 0;
					for(int j=ser_sep_index[i]+1; j < ser_sep_index[i+1]; j++){
						myargs[x++] = tokens[j];
					}

					if(execvp(myargs[0],myargs)!=0){
						printf("Shell: [Error] Invalid Command\n");
					}

					exit(0);
				}else{ //parent process reaps the children
					int status;
					waitpid(pid,&status,0);
				}

				continue;
			}
		}
		else if(ctr_par > 1){
			
			int pids[64] = {0};
			int index = 0;
			for(int i = 0; i<ctr_par;i++){
				//debugging command :
				if(strcmp(tokens[par_sep_index[i]+1],"debug") == 0){
					printf("Process Table Size : %d\n",bg_proc);
					printf("Running background processes : \n");
					for(int j = 0; j<MAX_BACKGROUND_PROCESSES; j++){
						printf("%d\n",bg_procs[j]);
					}
					reap_background_processes(bg_procs,&bg_proc);
					continue;
				}

				if(strcmp(tokens[par_sep_index[i]+1],"cd") == 0){

					if(strcmp(tokens[par_sep_index[i]+3],"&&&") != 0){
						printf("Shell: [Error] Too many arguments for cd\n");
						reap_background_processes(bg_procs,&bg_proc);
						continue;
						
					}

					if (chdir(tokens[par_sep_index[i]+2]) != 0) {
						printf("Shell: [Error] Invalid directory\n");
					}

					reap_background_processes(bg_procs,&bg_proc);
					continue;
				}

				if(strcmp(tokens[par_sep_index[i]+1], "exit") == 0){ 

					printf("Shell: Killing all background processes...\n");

					int ppid = getpid();

					for(int j=0; j<bg_proc; j++){
						kill(bg_procs[j], SIGKILL); //sends a SIGKILL signal to specific pid of bg process
						// One important fact is that SIGTERM terminates the process and allows some cleanup. SIGKILL would just kill it
					}
					
					reap_background_processes(bg_procs,&bg_proc); //Clears all the zombie processes

					printf("Shell: Freeing all dynamically allocated memory...\n");
					
					//free dynamically allocated memory
					for(int j=0;tokens[j]!=NULL;j++){
						free(tokens[j]);
					}
					free(tokens);

					printf("Shell: Exited successfully\n");
					exit(0);
				}

				int pid = fork(); //this return's the child's PID
				pids[index++] = pid;
				if(pid == 0){
					// printf("I am child. My PID is %d.\n",getpid());
					signal(SIGINT, SIG_DFL);

					char *myargs[MAX_NUM_TOKENS * sizeof(char *)]; //similar to tokens

					int x = 0;
					for(int j=par_sep_index[i]+1; j < par_sep_index[i+1]; j++){
						myargs[x++] = tokens[j];
					}

					if(execvp(myargs[0],myargs)!=0){
						printf("Shell: [Error] Invalid Command\n");
					}

					exit(0);
				}else{ //parent process reaps the children
					int status;
				}

				continue;
			}
			int status;
			int pid;

			for(int i = 0; i < index; i++){
				waitpid(-1,&status,0);
			}
			printf("Shell: Parallel execution completed\n");

		}
		
		else{
			//debugging command :
			if(strcmp(tokens[0],"debug") == 0){
				printf("Process Table Size : %d\n",bg_proc);
				printf("Running background processes : \n");
				for(i = 0; i<MAX_BACKGROUND_PROCESSES; i++){
					printf("%d\n",bg_procs[i]);
				}
				reap_background_processes(bg_procs,&bg_proc);
				continue;
			}

			if(strcmp(tokens[0],"cd") == 0){ // TODO : add recognition for ' '

				if(tokens[2] != NULL){
					printf("Shell: [Error] Too many arguments for cd\n");
					reap_background_processes(bg_procs,&bg_proc);
					continue;
				}

				if (chdir(tokens[1]) != 0) {
					printf("Shell: [Error] Invalid directory\n");
				}

				reap_background_processes(bg_procs,&bg_proc);
				continue;
			}

			if(strcmp(tokens[0], "exit") == 0){ 

				printf("Shell: Killing all background processes...\n");

				int ppid = getpid();

				for(i=0; i<bg_proc; i++){
					kill(bg_procs[i], SIGKILL); //sends a SIGKILL signal to specific pid of bg process
					// One important fact is that SIGTERM terminates the process and allows some cleanup. SIGKILL would just kill it
				}
				
				reap_background_processes(bg_procs,&bg_proc); //Clears all the zombie processes

				printf("Shell: Freeing all dynamically allocated memory...\n");
				
				//free dynamically allocated memory
				for(i=0;tokens[i]!=NULL;i++){
					free(tokens[i]);
				}
				free(tokens);

				printf("Shell: Exited successfully\n");
				exit(0);
			}

			int pid = fork(); //this return's the child's PID

			if(pid == 0){
				//printf("I am child. My PID is %d.\n",getpid());
				signal(SIGINT, SIG_DFL);

				char *myargs[MAX_NUM_TOKENS * sizeof(char *)]; //similar to tokens

				for(i=0; tokens[i]!=NULL; i++){
					myargs[i] = tokens[i];
				}

				if(execvp(myargs[0],myargs)!=0){
					printf("Shell: [Error] Invalid Command\n");
				}

				exit(0);
			}else{ //parent process reaps the children
				int status;
				waitpid(pid,&status,0);
			}
		}
		
		//check for any zombie processes and free them
		reap_background_processes(bg_procs,&bg_proc);

		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}
