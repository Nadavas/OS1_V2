// signals.cpp
// contains signal handler funtions
// contains the function/s that set the signal handlers

/*******************************************/
#include "signals.h"
extern pid_t cur_fg_pid; // holds the process id of the fg process
extern std::string cur_fg_cmd;	 // holds the process cmd of the fg process
extern int cur_fg_jid;

void sig_handler(int sig_number){
	if(sig_number==SIGINT){
		std::cout << "smash: caught ctrl-C" << std::endl;
	if (fg_empty()()) {
		if (!kill(cur_fg_pid, SIGKILL)) {
			std::cout << "smash: process " << cur_fg_pid << " was killed" << std::endl;
			fg_clean();
		}
		else {
			std::perror("smash error: kill failed");
		}
	}
	else {
		printf("smash > ");
		fflush(stdout);

	}
	}
	else if (sig_number==SIGSTOP){
			std::cout << "smash: caught ctrl-Z" << std::endl;
	if (fg_empty()) {
		if ((fg_pid, fg_cmd, Stopped,fg_job_id)) {
			if (!kill(fg_pid, SIGSTOP)) {
				std::cout << "smash: process " << fg_pid << " was stopped" << std::endl;
				fg_clear();
			}
			else {
				std::perror("smash error: kill failed");
			}
		}
		else {
			std::cerr << "smash error: addNewJob failed\n";
		}
	}
	else {
		printf("smash > ");
		fflush(stdout);
	}

}	
