//		commands.cpp
//********************************************
#include "commands.h"
#include "signals.h"
#include <fcntl.h>
//global vars
char* prev_path = NULL;
char* cur_path = NULL;
std::map<int, job, std::less<int>> jobs_list;
int last_job_id = 0;

pid_t cur_fg_pid = -1; 
std::string cur_fg_cmd;
int cur_fg_jid = -1;

void update_jobs_list()
{
	int res;
	pid_t child_pid;
	// Checks which sons are still running
	auto it = jobs_list.begin();
	while (it != jobs_list.end())
	{
		res = 0;
		child_pid = waitpid((it->second).pid,&res, WNOHANG|WUNTRACED|WCONTINUED);
		if (child_pid == -1) //waitpid failed
		{
			std::perror("smash error: waitpid failed");
			return;
		}

		if(WIFSTOPPED(res))
		{
			(it->second).is_stopped = true;

		}
		else if(WIFCONTINUED(res))
		{
			(it->second).is_stopped = false;
		}
		else if(child_pid == (it->second).pid) 
		{
			jobs_list.erase(it);
		}
		it++;
	}
	// Update last_job array to match the last job in jobs (with the biggest job-id)
	if (jobs_list.empty()) {
		last_job_id = 0;
	}
	else{
		last_job_id = jobs_list.rbegin()->first;
	}
}

bool check_if_built_in_cmd(std::string command) {
	const std::string built_in_commands[] =
	{ "showpid" ,"pwd","cd","jobs","kill","fg","bg","quit","diff" };
	if (command[command.length() - 1] == '&') {
		command.pop_back();		//removes &
		if (command.empty()) {
			return false;
		}
	}
	// Search in 'BUILT_IN_CMD' the cmd
	return (std::find(std::begin(built_in_commands), 
	std::end(built_in_commands), command) != std::end(built_in_commands));
}

bool fg_empty() {
	if ((cur_fg_pid != -1) && (!cur_fg_cmd.empty())) {
		return true;
	}
	return false;
}

void fg_clean() {
	cur_fg_pid = -1;
	cur_fg_cmd.clear();
	cur_fg_jid = -1;
}

void fg_replace(pid_t pidx, std::string cmdx, int jidx) {
	cur_fg_pid = pidx;
	cur_fg_cmd = cmdx;
	cur_fg_jid = jidx;
}

//// JOB METHODS ////
job::job():pid(-1),cmd(""),is_stopped(true),entered_time(time(NULL)){}
job::job(pid_t pidx,std::string cmdx,bool is_stoppedx,time_t entered_timex)
{
		pid=pidx;
		cmd=cmdx;
		is_stopped=is_stoppedx;
		entered_time=entered_timex;
}
job::job(const job& new_job) {
	pid = new_job.pid;
	cmd = new_job.cmd;
	is_stopped = new_job.is_stopped;
	entered_time = new_job.entered_time;
}
job& job::operator=(const job& new_job) {
	if (this == &new_job) {
		return *this;
	}
	pid = new_job.pid;
	cmd = new_job.cmd;
	is_stopped = new_job.is_stopped;
	entered_time = new_job.entered_time;
	return *this;
}
job::~job(){}

int find_stopped_job()
{
	bool flag = false;
	int highest_jid_stopped;
	auto it=jobs_list.begin();
	while(it!=jobs_list.end()){
		if(it->second.is_stopped){
			highest_jid_stopped = it->first;
			flag=true;
		}
		it++;
	}
	if (!flag)
		return -1;
	return highest_jid_stopped;
}

int break_cmd_to_args(std::string input, std::string(&args)[MAXARGS], std::string delim)
{

	char* iter = strtok(const_cast<char*>(input.c_str()), const_cast<char*>(delim.c_str()));
	int i = 0, num_arg = 0;
	if (iter == NULL)
		return 0;
	args[0] = iter; 
	for (i = 1; i < MAXARGS; i++)
	{
		iter = strtok(NULL, const_cast<char*>(delim.c_str()));
		if (iter != NULL) {
			args[i] = iter;
			num_arg++;  
		}

	}
	return num_arg;
}

//********************************************
// function name: ExeCmd
// Description: interperts and executes built-in commands
// Parameters: pointer to jobs, command string
// Returns: 0 - success,1 - failure
//********************************************************
int ExeCmd(std::string args[MAXARGS], int args_count, std::string command)
{	
	bool illegal_cmd = false; // illegal command
/*************************************************/
// Built in Commands PLEASE NOTE NOT ALL REQUIRED
// ARE IN THIS CHAIN OF IF COMMANDS. PLEASE ADD
// MORE IF STATEMENTS AS REQUIRED
/*************************************************/

	if (args[0] == "cd")
	{
		// Cd error checks
		if (args_count > 1)  // too many arguments
			std::cerr << "smash error: cd: too many arguments\n";
		else if (args[1] == "-") {
			if (prev_path == NULL) {
				 // there is no last path specified yet since the start of the program
				std::cerr << "smash error: cd: OLDPWD not set\n";
				illegal_cmd = true;
			}
			else if (chdir(prev_path)) {
				perror("smash error: chdir failed");
				illegal_cmd = true;
			}
			else {
				// In case of successful cd - Update last path by swap current<->last paths
				char* temp = cur_path;
				cur_path = prev_path;
				prev_path = temp;
			}
		}
		else {
			//
			if (chdir(const_cast<char*>(args[1].c_str()))){
				perror("smash error: chdir failed");
			}
			else {
				// In case of successful cd - Update last path
				char* temp = get_current_dir_name();
				if (temp == NULL) {
					perror("smash error: getcwd failed");
					return -1;
				}
				free(prev_path);
				prev_path = cur_path;
				cur_path = temp;
			}
		}
	} 

	
	/*************************************************/
	else if (args[0] == "pwd")
	{
		if (cur_path) {
			std::cout << cur_path << std::endl;
		}
		else {
			char* temp = get_current_dir_name();
			if (temp == NULL) {
				perror("smash error: getcwd failed");
				return -1;
			}
			else { 
				// print the pwd
				std::cout << temp << std::endl; 
				cur_path = temp;
			}
		}
	}
	
	/*************************************************/
	else if (args[0] == "kill")
	{
		// Valid arguments check
		if (args_count != 2) {
			std::cerr << "smash error: kill: invalid arguments\n";
			illegal_cmd = true;
		}
		else{
			std::string signum_s(args[1]);
			std::string rel_jid(args[2]);
			if (signum_s[0] != '-'){ // kill's first argument must start with '-'
				std::cerr << "smash error: kill: invalid arguments\n";
				illegal_cmd = true;
			}
			else signum_s.erase(0,1); // erase the '-' in kill's first arg
			if (!illegal_cmd && (!is_number(signum_s) || !is_number(rel_jid) )){
				// one/both of the kill's arguments are not integers - error
				std::cerr << "smash error: kill: invalid arguments\n";
				illegal_cmd = true;
			}
			else if(!illegal_cmd){
				// job_id check
				int job_id_int = std::stoi(rel_jid);
				int signum_int = std::stoi(signum_s);
				// Search the job-id in the jobs map
				if(jobs_list.find(job_id_int) == jobs_list.end()){
					// The job-id entered is not found in the jobs map - error
					std::cerr << "smash error: kill: job-id " <<job_id_int<<" does not exist" << std::endl;
					illegal_cmd = true;
				}
				else {
					if (kill(jobs_list[job_id_int].pid , signum_int)){
						std::perror("smash error: kill failed");
						illegal_cmd = true;
					}
					// Signal successfully sent
					std::cout << "signal number "<< signum_int<< " was sent to pid "<<jobs_list[job_id_int].pid << std::endl;
				}
			}
		}
	}
	else if (args[0] == "diff")
	{
		if (args_count != 2) {
            std::cerr << "smash error: diff: invalid arguments\n";
            illegal_cmd = true;
        } 
		else{
            int f1, f2;
            f1 = open(args[1].c_str(), O_RDONLY);
            if (f1 == -1) {
                perror("smash error: open failed");
                return -1;
            }
            f2 = open(args[2].c_str(), O_RDONLY);
            if (f2 == -1) {
                perror("smash error: open failed");
                close(f1);	
                return -1;
            }
            char c1, c2;
            bool diff = false;
            // Checks every byte in the files 
            while (read(f1, &c1, 1) > 0 || read(f2, &c2, 1) > 0) {
                if (c1 != c2) {
                    diff = true;
                    break;
                }
            }

            if (diff) { // different
                std::cout << "1" << std::endl;
            } 
			else { // identical
                std::cout << "0" << std::endl;
            }

            close(f1);
            close(f2);
        }
	}
	/*************************************************/
	
	else if (args[0] == "jobs")
	{
		// Update the state(Running/Stopped) of every job in the jobs map
		update_jobs_list();
		time_t present_time(time(NULL));
		if (present_time == -1) {
			std::perror("smash error: time failed");
			return -1;
		}
		double diff_time;
		for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it){
			// Prints the details of every job in the jobs map
			 diff_time = difftime(present_time, (it->second).entered_time);
			 std::cout << "[" << it->first << "] " << (it->second).cmd
					 << " : " << (it->second).pid << " "
					 << diff_time << " secs";
			 if((it->second).is_stopped){
				 std::cout << " (stopped)";
			 }
			 std::cout << std::endl;
		}
	}
	/*************************************************/
	else if (args[0] == "showpid")
	{
		std::cout << "smash pid is " << getpid() << std::endl;
	}
	/*************************************************/
	else if (args[0] == "fg")
	{
		if(args_count > 1){
			// too many arguments - error
			std::cerr << "smash error: fg: invalid arguments" << std::endl;
			illegal_cmd = true;
		}
		else if (args_count == 0 && jobs_list.empty()) {
			// no arguments passed, and map is empty - error
			std::cerr << "smash error: fg: jobs list is empty" << std::endl;
		}
		else if (args_count != 0 && !is_number(args[1])){
			// argument is not a number - invalid argument - error
			std::cerr << "smash error: fg: invalid arguments" << std::endl;
		}
		else if ((args_count != 0) && (search_job(args[1])==-1)) {
			// job id is not found in the jobs map - error
			std::cerr << "smash error: fg: job-id " << args[1] << " does not exist" << std::endl;
		} else {
			// no error
			int job_id = -1;
			if (args_count == 0) { // take the biggest job id
				job_id = jobs_list.rbegin()->first; // the biggest key
			}
			else job_id = search_job(args[1]); // take the arg job_id
			// Update which job is currently in foreground state
			fg_clean();
			fg_replace(jobs_list[job_id].pid, jobs_list[job_id].cmd, job_id);
			std::cout << jobs_list[job_id].cmd << " : " << jobs_list[job_id].pid << std::endl;
			jobs_list.erase(job_id);

			if (kill(cur_fg_pid, SIGCONT)) {
				std::perror("smash error: kill failed");
				return -1;
			}
			// wait for job to finish - running in foreground
			if (waitpid(cur_fg_pid, NULL, WUNTRACED) == -1) {
				std::perror("smash error: waitpid failed");
				return -1;
			}
			// after wait_pid finishes - the job no longer runs in the foreground
			fg_clean();
		}
	} 
	/*************************************************/
	else if (args[0] == "bg") //
	{
		if(args_count > 1){
			// too many arguments - error
			std::cerr << "smash error: bg: invalid arguments" << std::endl;
			illegal_cmd = true;
		}
		else if (args_count == 0 && (find_stopped_job() == -1)) {
			// no arguments passed, no stopped job in jobs map - error
			std::cerr << "smash error: bg: there are no stopped jobs to resume" << std::endl;
			illegal_cmd = true;
		}
		else if (args_count != 0 && !is_number(args[1])){
			// argument is not a number - invalid argument - error
			std::cerr << "smash error: bg: invalid arguments" << std::endl;
			illegal_cmd = true;
		}
		else if ((args_count != 0) && (search_job(args[1])==-1)) {
			// job id is not found in the jobs map - error
			std::cerr << "smash error: bg: job-id " << args[1] << " does not exist" << std::endl;
			illegal_cmd = true;
		}
		else {
			int job_id = 0;
			// job_id is the job in arg[1] and if it wasn't entered by the user its the
			// last stopped job in the jobs map
			if(args_count == 0){
				job_id = find_stopped_job(); // last stopped job
			}
			else {
				job_id = std::stoi(args[1]); // entered job
			}
			if (jobs_list[job_id].is_stopped == false){
				// this job is already running - error
				std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << std::endl;
			}
			else {
				std::cout << jobs_list[job_id].cmd << " : " << jobs_list[job_id].pid << std::endl;
				if (kill(jobs_list[job_id].pid, SIGCONT)) {
					std::perror("smash error: kill failed");
					return -1;
				}
				jobs_list[job_id].is_stopped = false;
			}
		}
	}
	/*************************************************/
	else if (args[0] == "quit")
	{
		pid_t child_pid;
   		if(args[1] == "kill"){
   			// kill every job in jobs map. at first send SIGTERM signal,
   			// if after 5 seconds it wasn't killed - send SIGKILL signal
   			for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it){ // for each of the jobs running
				std::cout << "[" << it->first << "] " << it->second.cmd << " - " <<"Sending SIGTERM...";
   				if(kill(it->second.pid, SIGTERM)){
   					// kill error
   					std::perror("smash error: kill failed");
   					return -1;
   				}
   				
   				fflush( stdout);
   				// wait 5 seconds
   				sleep(5);
   				// Check if the process was killed by SIGTERN signal
   				child_pid = waitpid(it->second.pid, NULL, WNOHANG);
   				if ( child_pid == -1) {
   					// waitpid error
   					std::perror("smash error: waitpid failed");
   					return -1;
   				}
   				if( child_pid != it->second.pid){
   					// the process wasn't killed yet - send SIGKILL
   					std::cout << " (5 sec passed) Sending SIGKILL…";
   					if (kill(it->second.pid, SIGKILL)){
   						std::perror("smash error: kill failed");
   						return -1;
   					}
   				}
   				std::cout << " Done." << std::endl;
			  }
   		}
		free(prev_path);
		free(cur_path);
   		exit(0);
	} 
	/*************************************************/
	if (illegal_cmd == true)
	{
		return -1;
	}
    return 0;
}
//**************************************************************************************
// function name: ExeExternal
// Description: executes external command
// Parameters: external command arguments, number of actual arguments, and external command string.
// Returns: Success or Failed
//**************************************************************************************
int ExeExternal(std::string args[MAXARGS], int args_count, std::string command)
{
	pid_t pID;
	switch(pID = fork()) {
		case -1:
					// fork failed error
					std::perror("smash error: fork failed");
					return -1;

		case 0 :
                	// Child Process
					char* argv[MAXARGS];
					// Arrange the args in argv[] for execv()
					for (int i = 0; i < args_count + 1; i++)
						argv[i] = const_cast<char*>(args[i].c_str());
					argv[args_count + 1] = NULL;
					setpgrp();
					execv(argv[0], argv);
					// if we got here it means the execv failed
					std::perror("smash error: execv failed");
					exit(1);

		default:
                	// Parent process - meaning the external cmd was sent to be executed
					fg_clean();
					fg_replace(pID, command);
					// wait for the command to finish in foreground
					if (waitpid(pID, NULL, WUNTRACED) == -1) {
						std::perror("smash error: waitpid failed");
						return -1;
					}
					// the command finished
					fg_clean();
					return 0;
	}
	return -1;
}

//**************************************************************************************
// function name: BgCmd
// Description: if command is in background, insert the command to jobs
// Parameters: external background command arguments, number of actual arguments, and external background command string.
// Returns: Success or Failed
//**************************************************************************************
int BgCmd(std::string args[MAXARGS], int args_count, std::string command)
{		
	pid_t pID;
	switch(pID = fork()){
			case -1:
					std::perror("smash error: fork failed");
					return -1;

			case 0 :
					// Child Process
					setpgrp();
					char * argv[MAXARGS];
					for (int i = 0; i < args_count + 1; i++)
						argv[i] = const_cast<char*>(args[i].c_str());
					argv[args_count + 1] = NULL;
					execv(argv[0], argv);
					std::perror("smash error: execv failed");
					exit(1);

			default:
					// Parent process - meaning the external cmd was sent to be executed
					if (!insert_job(pID, command)) {
						return -1;
					}
					return 0;
	}
	return -1;
}


//********************************************
// function name: is_number
// Returns: bool 'true' if the string entered is a number
//********************************************
bool is_number(std::string& str)
{
    for (char const &c : str) {
        // using the std::isdigit() function
        if (std::isdigit(c) == 0)
          return false;
    }
    return true;
}


//********************************************
// function name: arg_in_map
// Description: Searches whether the job-id entered( in string format)
// is in the jobs map. It is called after making sure this arg is a valid number.
// Returns: the int job-id if it was found, -1 otherwise
//********************************************
int search_job(std::string& arg){
	int num = std::stoi(arg);
	if (jobs_list.find(num) == jobs_list.end())
		return -1;
	return num;
}

/*
	addNewJob:
		By a given process, whether it has already a job ID or not.
		The function will insert the process as a new job object into the jobs ADT.
		returns: boolean state, if the operation succeeded.
*/
bool insert_job(pid_t pID, std::string cmd, bool is_stopped, int job_id) {
	update_jobs_list();
	time_t curr_time(time(NULL));
	if (curr_time == -1) {
		std::perror("smash error: time failed");
		return false;
	}
	job newjob(pID, cmd, is_stopped, curr_time);
	int final_job_id = last_job_id + 1;
	if (job_id != -1) {
		final_job_id = job_id;
	}
	bool result = jobs_list.insert(std::pair<int, job>(final_job_id, newjob)).second;
	return result;
}
