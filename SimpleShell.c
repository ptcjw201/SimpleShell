#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#define MAX_LINE 80 /* The maximum length command */

/*because fgets function gets an input with a "\n"
you have to remove "\n" before using args2[0] in redirections*/
char* removenl(char *a){
    a[strcspn(a,"\n")] = '\0';
    return a;
}

void normalcommand(char *args[], int count, bool do_wait){	
	int ind = 0;
	int out;
	/*args2 gets string before the point ">" */
	int pid, status;
	int i = 0;

	/*change "|" to NULL*/
	if(do_wait == false){
		args[count-1] = NULL;
	}

	/*make pid*/
	pid = fork();
	if(pid == -1){
		perror("Failure");
	}
	else if(pid == 0){
		execvp(args[0], args);
		exit(2);
	}
	else{
		/*wait only if there is no &*/
		if(do_wait){
			pid = wait(&status);
		}
	}
}


void makepipe(char *args[], int count, int ispip, bool do_wait){	
	int ind = 0;
	int out;
	char *args2[MAX_LINE/2 + 1];
	int pid1,pid2, status;
	int temp = ispip;
	int i = 0;
	/*  after the while loop 
		args2 is the part before "|",
		args is the part after "|"*/
	while(ind<count-ispip){
		args2[ind] = args[ispip+1];
		args[ispip+1] = NULL;
		ind++;
		ispip++;
	}
	/*put NULL at the last part */
	args2[ind] = NULL;
	/*change "|" to NULL */
	args[temp] = NULL;


	/*make pipeline*/
    int fd[2];
    if(pipe(fd) == -1) {
        perror("Pipe failed");
        exit(1);
    }
    /*fork a child process*/
    pid1 = fork();
    if(pid1 == -1){
      perror("Failure");
    }
    if(pid1 == 0){            
      /*does the child part which is args*/
        close(fd[0]);    
        dup2(fd[1], 1);        
        close(fd[1]);
        execvp(args[0], args);
    }
    else{ 
    	/*fork a child process*/
    	pid2 = fork();
    	if(pid2 == -1){
    		perror("Failure");
    	}
    	if(pid2 == 0){	
    		close(fd[1]);  
        	dup2(fd[0], 0);      
        	close(fd[0]);
        	execvp(args2[0], args2);
        }
        else{
        	/*after completing task, you should close open pipes*/
        	close(fd[1]);
        	close(fd[0]);
        	/*wait only if do_wait is true(=no &)*/
        	if(do_wait){
        		wait(NULL);
        		wait(NULL);
        	}
        }
    }
}

/*redirection output by ">" */
void outred(char *args[], int count, int isout, bool do_wait){
	int ind = 0;
	int out;
	/*args2 gets string before the point ">" */
	char *args2[MAX_LINE/2 + 1];
	int pid, status;

	/*removes & from the end of args*/
	if(do_wait == false){
		args[count-1] = NULL;
	}
	while(ind<count-1){
		/*remove > from args*/
		args[isout] = NULL;
		args2[ind] = args[isout+1];
		args[isout+1] = NULL;
		isout++;
		ind++;
	}
	/*fork a child process using fork()*/
	pid = fork();
	if(pid == -1){
		perror("Failure");
	}
	else if (pid == 0){		
		/*O_CREAT, O_WRONLY, O_TRUNC : when there isn't a file, open by write-only*/
		out = open(removenl(args2[0]), O_CREAT | O_WRONLY | O_TRUNC, 0644);
		/*redirect standard output to our out */
		dup2(out, STDOUT_FILENO);
		close(out);
		execvp(args[0], args);
	}
	else{
		/*wait only if there is no &*/
		if(do_wait){
			pid = wait(&status);
		}
	}
}

/*redirection input by "<" */
void intred(char *args[], int count, int isin, bool do_wait){
	int ind = 0;
	int in;
	/*args2 gets string before the point "<" */
	char *args2[MAX_LINE/2 + 1];
	int pid, status;

	/*removes & from the end of args*/
	if(do_wait == false){
		args[count-1] = NULL;
	}
	while(ind<count-1){
		/*remove < from args*/
		args[isin] = NULL;
		args2[ind] = args[isin+1];
		args[isin+1] = NULL;
		isin++;
		ind++;
	}
	/*fork a child process using fork()*/
	pid = fork();
	if(pid == -1){
		perror("Failure");
	}
	else if(pid == 0){
		/*below is the code to check whether 
		  if the parent doesn't wait for the child,
		  was used for test*/
		/*
		if(!do_wait){
			sleep(1);
		}
		*/
		/*O_RDONLY : when opening by read-only*/
		in = open(removenl(args2[0]), O_RDONLY);
		/*when there isn't the file, exit(1)*/
		if(in == -1){
			exit(1);
		}
		/*redirect standard input to our in */
		dup2(in, STDIN_FILENO);
		close(in);
		execvp(args[0], args);
	}
	else{
		/*wait only if there is no &*/
		if(do_wait){
			pid = wait(&status);
		}
	}
}




int main(void){
	char *args[MAX_LINE/2 + 1]; /* command line arguments */
	char *args2[MAX_LINE/2 + 1];
	int should_run = 1; /* flag to determine when to exit program */
	char input[MAX_LINE]; /*your command input */
	int count = 0; /*number of parces strings */
	char *parce; /* parameters for redirection, pipe and else */

    while (should_run){ 
    	/*variables for deciding which type of command*/
		int ispip = 0;
		int isout = 0;
		int isin = 0;
		bool do_wait = true;

    printf("OSS> ");
		fflush(stdout);
		count = 0;
		/*gets input of command*/

		fgets(input, MAX_LINE, stdin);
		/*parcing by empty space*/
		parce = strtok(input, " ");
		while(parce != NULL){
			/* see if there are delimiters such as >, <, | 
			if there is, remember the index */

			args[count] = parce;
			if(strcmp(parce, ">") == 0){
				isout+=count;
			}
			else if(strcmp(parce, "<") == 0){
				isin+=count;
			}
			else if(strcmp(parce, "|") == 0){
				ispip+=count;
			}

			parce = strtok(NULL," ");
			/*count => strlen*/
			count += 1;
		}
		args[count] = NULL;

		/*check for "&"*/
		for(int x = 0;x<count;x++){
			/*if you get inputs by fgets, there is always a "\n" at the end
			for comparison, used removenl to remove "\n*/
			if(strcmp(removenl(args[x]), "&")==0){
				do_wait = false;
			}
		}

		/*exits if "exit" is entered*/
		if(strcmp(args[0], "exit") == 0){
			should_run = 0;
			break;
		}

		/*executes functions according to ispip, isin, isout
		passes the parameter do_wait*/
		if(ispip != 0){
			makepipe(args, count, ispip,do_wait);
		}
		else if(isout != 0){
			outred(args, count, isout, do_wait);
		}
		else if(isin !=0){
			intred(args, count, isin, do_wait);
		}
		else{
			normalcommand(args, count, do_wait);
		}
	}
	return 0;
}
