/*	smash.cpp
main file. This file contains the main function of smash
*******************************************************************/

#include "commands.h"
#include "signals.h"
#define MAX_LINE_SIZE 80
#define MAXARGS 20

//**************************************************************************************
// function name: main
// Description: main function of smash. get command from user and calls command functions
//**************************************************************************************
int main(int argc, char *argv[])
{	
	//signal declaretions
	//NOTE: the signal handlers and the function/s that sets the handler should be found in siganls.c
	 /* add your code here */
	struct sigaction sa = { 0 };
	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	/************************************/
	//NOTE: the signal handlers and the function/s that sets the handler should be found in siganls.c
	//set your signal handlers here
	/* add your code here */
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("smash error: sigaction failed");
	}
	if(sigaction(SIGTSTP, &sa, NULL) == -1){
		perror("smash error: sigaction failed");
	}
	/************************************/
	//check for current_path
	if (!cur_path && !prev_path) { 
		char* temp = get_current_dir_name();
		if (temp == NULL) {
			perror("smash error: getcwd failed");
			return -1;
		}
		cur_path = temp;	// set cur_path to the cwd
	}
	/************************************/
    while (1){
    	printf("smash > ");
		update_jobs_list();		// have to do that between every 2 commands
		std::string input,command;
		std::getline (std::cin,input);
		command = input;		// save input so we can manipulate it
		std::string args[MAXARGS];
		int args_count = break_cmd_to_args(input, args);	// args - array of cmd arguments	
		if (check_if_built_in_cmd(args[0])) {
			// in case of built in command
			char last_char_cmd = args[args_count][args[args_count].length()-1];
			if (last_char_cmd == '&') {
				args[args_count].pop_back();	//remove &
				if (args[args_count].empty()) {
					args_count--;
				}
			}
			// running built in commands
			ExeCmd(args, args_count, command);
		}
		else {
			// in case of external command
			char last_char_cmd = args[args_count][args[args_count].length()-1];
			if (args[0]!="&" && last_char_cmd == '&') {
				args[args_count].pop_back();	//remove &
				if (args[args_count].empty()) {
					args_count --;
				}
				BgCmd(args, args_count, command);	// runs it in bg
			}
			else {
				ExeExternal(args, args_count, command);		// in fg
				
			}
		}
		fg_clean();

	}
	free(prev_path);
	free(cur_path);
    return 0;
}

