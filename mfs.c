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

//execute function that executes the user typed command.
// runs all the commands from the current directory, /usr/local/bin , /usr/bin , /bin
void execute_func(char**token,int p_count,int c_pids[]);

void history_printer(int c_count, char history[100][MAX_COMMAND_SIZE]);

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  int c_pids[100];
  char history [100][MAX_COMMAND_SIZE];
  int c_count = 0;
  int p_count =0;
  int i;
  while( 1 )
  {
  	
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    // checks for the empty input and restarts the while loop.
   	if(strcmp(cmd_str,"\n")==0) continue;

   	// Adds the typed input in the history array for future execution.
   	strcpy(history[c_count], cmd_str); 
   	//Updates the number of command typed during the session.
   	c_count++;

   	if(cmd_str[0]=='!')
    {
    	char history_num[10];
    
    	for (int i=1;i<strlen(cmd_str);i++)
    	{
    	    history_num[i-1] = cmd_str[i]; 
    	}
    	history_num[i-1] = '\0';

    	int choice_run = atoi(history_num);

		strcpy(cmd_str,history[choice_run]);    
    }
	
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

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }

    free( working_root );

    if(strcmp(token[0],"exit")==0 || strcmp(token[0],"quit")==0)
    {
    	fflush(NULL);
    	exit(0);
    }
    else if(strcmp(token[0],"cd")==0)
    {
    	chdir(token[1]);
    }
    
    else if(strcmp(token[0],"history")==0)
    {
		history_printer(c_count, history); 
		// calls history printing function that prints all the past typed command.
    	
    }
    else if(strcmp(token[0],"listpids")==0 || strcmp(token[0],"showpids")==0)
    {
    	for(i=0;i<p_count;i ++)
    	{
    		printf("%d: %d\n",i,c_pids[i]);	
    	}
    }
    else
    {
    	int semicolon_detect =0;
    	for(i=0;i<token_count-1;i++)
    	{

    		if(strcmp(token[i],";")==0)
    		{
    			semicolon_detect = 1;
    		}
    	}
    	if(semicolon_detect ==1)
    	{



    	}
    	else
    	{
    		execute_func(token,p_count,c_pids);
    	}	
  }

}

return 0;
}

//execute function that executes the user typed command.
// runs all the commands from the current directory,
//		/usr/local/bin
//		/usr/bin
//		/bin


void execute_func(char**token,int p_count,int c_pids[])
{

	int child_pid = fork();
    	int child_status;

    	if(child_pid ==0)
    	{
    		child_status = execvp(token[0],token);
    		if(child_status!=0)
    		{
    			printf("%s: Command not found.\n\n",token[0]);
    		}
    		exit(0);
    	}
    	if(child_pid !=0 && child_status ==0)
    	{
    		c_pids[p_count] = child_pid; 
    		p_count++;	
    	}

    	wait(&child_status);

    }

void history_printer(int c_count, char history[100][MAX_COMMAND_SIZE])
{
	int i;
	if(c_count<=15)
    	{
    		for(i=0;i<c_count;i ++)
    		{
    			printf("%d: %s",i,history[i]);	
    		}
    	}
    	else
    	{
    		for(i=c_count-15;i<c_count;i ++)
    		{
    			printf("%d: %s",i,history[i]);	
    		}
    	}
}

