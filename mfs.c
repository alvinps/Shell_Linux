/*
	Name: Alvin Poudel Sharma
	ID: 1001555230



*/




// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

//  execute function that executes the user typed command.
//  runs all the commands from the current directory,
//  /usr/local/bin , /usr/bin , /bin
//	also update the process count and store the child pids in the array.
void execute_func(char**token,int* p_count,int c_pids[]);

//	prints the last 15 command typed by the user for shortcuts(i.e !1  !2 !3 ans so on.)
void history_printer(int c_count, char history[100][100]);

//	prints the lists process ids of last 15 child processes executed from this shell.
void pids_printer(int p_count, int c_pids[]);

//	handles the signals (Ctrl-Z and Ctrl-C )
//	is  just a handler and does nothing.
static void handle_signal (int sig );

//	revives the last suspended child process by the user.
//	the pid of the suspended child process to be awaken is passed. 
void revive_process(int child_pid);

//	this function deals with tokenizing the command string provided.
//	after tokenizing it passes the tokens to execute_func for further execution.
//	It also adds the pid of a child created by the shell. 
//	this function also tracks down the total child process created.
void shell_helper(char* cmd_str,int*c_count,int* p_count,int c_pids[] ,char history[100][100]);

int main()
{

	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
	// array to store child process ids
	int c_pids[100];
	char history [100][100];
	
	// counters for processes and history.
	int c_count = 0;
	int p_count =0;
	
	int i;

	struct sigaction sig_processing;

  //  Zero out the sigaction struct
	memset (&sig_processing, '\0', sizeof(sig_processing));

	sigfillset(&sig_processing.sa_mask);

  //  Set the handler to use the function handle_signal()
	sig_processing.sa_handler = &handle_signal;

	if (sigaction(SIGINT , &sig_processing, NULL) < 0) {
		perror ("sigaction: SIGINT");
		return 1;
	}

	if (sigaction(SIGTSTP , &sig_processing, NULL) < 0) {
		perror ("sigaction: SIGTSTP");
		return 1;
	}

	while( 1 )
	{
		fflush(NULL);
    // Print out the msh prompt
		printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input

		fflush(NULL);
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    // checks for the empty input and restarts the while loop.
		if(strcmp(cmd_str,"\n")==0) continue;

   	// Adds the typed input in the history array for future execution.
		strcpy(history[c_count], cmd_str); 
   	//Updates the number of command typed during the session.
		c_count++;

		//	checks for '!' so that specific command from the
		//	history. 
		if(cmd_str[0]=='!')
		{
			char history_num[10];
			//	copies the characters after '!' for converting into int.
			for (i=1;i<strlen(cmd_str);i++)
			{
				history_num[i-1] = cmd_str[i]; 
			}

			history_num[i-2] = '\0';
			//	just to make the conversion correct, the last character of the string
			//	is nulled, Note: atoi will give 0 otherwise.
			int choice_run = atoi(history_num);
			// checks the bound of the typed comment if it is within (1-nth) or not. 
			if(choice_run>c_count|| choice_run<=0)
			{
				printf("Command not in history.\n");
				continue;

			}
			if(c_count>=15)
			{
				choice_run = (c_count-16)+choice_run;
			}
			// copies the cmd from history for execution.
			strcpy(cmd_str,history[choice_run-1]);
		}

		//	this section of code detects the presence of semicolon or not.
		int semicolon_detect =0;
		for(i=0;i<strlen(cmd_str);i++)
		{
			if(cmd_str[i]==';')
			{
				semicolon_detect = 1;
			}
		}

		//	on detecting a ";" the cmd string is first tokenized into multiple command str.
		//  then they are passed accordingly until all the commands are executed.
		if(semicolon_detect ==1)
		{
			char * token = strtok(cmd_str,";");
			while(token!= NULL)
			{
				// tokenize the given string and executes the command provided by the user.
				shell_helper(token, &c_count, &p_count, c_pids ,history );

				token = strtok(NULL,";");
				if(token ==NULL) break;
				if(token[0]==' ') token++;
				//	note: during execution of multiple cmd seperated by ";" the presence of 
				//	space caused segfault during exec.So, token is moved up to prevent it.
			}					
		}
		else
		{
			// tokenize the given string and executes the command provided by the user.
			shell_helper(cmd_str, &c_count, &p_count, c_pids ,history );
		}
		fflush(NULL);
	}

	return 0;
}

//execute function that executes the user typed command.
// runs all the commands from the current directory,
//		/usr/local/bin
//		/usr/bin
//		/bin
void execute_func(char**token,int* p_count,int c_pids[])
{
	// forking
	int child_pid = fork();
	int child_status;

	if(child_pid ==0)
	{
		//	running exec function by passing the provided token
		//	if command is not found it would return -1 that is handled later on.
		child_status = execvp(token[0],token);
		// handles the command not found condition.
		if(child_status!=0)
		{
			printf("%s: Command not found.\n\n",token[0]);
		}
		exit(0);
	}
	// updates the process table 
	c_pids[*p_count] = child_pid;
	// updates the executed child process count 
	*p_count= *p_count + 1;	

	//	parent process waits for its child to end.
	waitpid(child_pid , &child_status, 0 );
	fflush(NULL);
}

//	 prints the command histroy typed by the user.
void history_printer(int c_count, char history[100][100])
{
	int i;
	if(c_count<=15)
	{
		//	when the number of commands are less than 15
		for(i=0;i<c_count;i ++)
		{
			printf("%d: %s",i+1,history[i]);	
		}
	}
	else
	{
		//	when the number of commands are more than 15
		//	 accomodation made to just print last 15 recent commands
		for(i=c_count-15;i<c_count;i ++)
		{
			printf("%d: %s",i-(c_count-15)+1,history[i]);	
		}
	}
}

//	prints the lists process ids of last 15 child processes executed from this shell.
void pids_printer(int p_count, int c_pids[])
{
	int i;
	if(p_count<=15)
	{
		//	when the number of processes are less than 15
		for(i=0;i<p_count;i ++)
		{
			printf("%d: %d\n",i+1,c_pids[i]);	
		}
	}
	else
	{	//	when the number of processes are more than 15
		//	 accomodation made to just print last 15 recent processes
		for(i=p_count-15;i<p_count;i ++)
		{
			printf("%d: %d\n",i-(p_count-15)+1,c_pids[i]);	
		}
	}
}


void revive_process(int child_pid)
{
	//	resumes the suspended child process by sending the signal.
	kill(child_pid,SIGCONT);
}

//	handles the signals (Ctrl-Z and Ctrl-C )
//	is  just a handler and does nothing.
static void handle_signal (int sig )
{
  // Empty to just catch the signal and do nothing.
}


//	this function deals with tokenizing the command string provided.
//	after tokenizing it passes the tokens to execute_func for further execution.
//	It also adds the pid of a child created by the shell. 
//	this function also tracks down the total child process created.
void shell_helper(char* cmd_str,int*c_count,int* p_count,int c_pids[] ,char history[100][100])
{

	  /* Parse input */
	char *token[MAX_NUM_ARGUMENTS];

	int   token_count = 0;                                 

    // Pointer to point to the token
    // parsed by strsep
	char *arg_ptr;                                         

	char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
	char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
	while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
		(token_count<MAX_NUM_ARGUMENTS))
	{
		token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
		if( strlen( token[token_count] ) == 0 )
		{
			token[token_count] = NULL;
		}
		token_count++;
	}

	free( working_root );

	//exits the shell if user types exit or quit.
	if(strcmp(token[0],"exit")==0 || strcmp(token[0],"quit")==0)
	{
		fflush(NULL);
		exit(0);
	}

	// deals with all sorts of cd command 
	else if(strcmp(token[0],"cd")==0)
	{
		chdir(token[1]);
	}
	//	resumes the last suspended child process.
	//	note: if a different command is executed after suspending the process,
	//	the suspended can't be awaken later.
	else if(strcmp(token[0],"bg")==0)
	{
		revive_process(c_pids[*p_count-1]);
	}
	// prints the history of cmd typed
	else if(strcmp(token[0],"history")==0)
	{
		history_printer(*c_count, history); 
		// calls history printing function that prints all the past typed command.
	}
	//	prints the list of processes executed.
	else if(strcmp(token[0],"listpids")==0 || strcmp(token[0],"showpids")==0)
	{
		pids_printer(*p_count, c_pids);
	}
	else
	{
		//	executes the token provided and updates the pid list and its count.
		execute_func(token,p_count,c_pids);
	}
}
