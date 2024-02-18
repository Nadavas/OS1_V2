#ifndef _COMMANDS_H
#define _COMMANDS_H
#include <unistd.h> 
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <cstring>
#include <map>
#include <algorithm>    
#include <cerrno>
#include <cstdio>
#include <iostream>

#define MAX_LINE_SIZE 80
#define MAXARGS 20

extern pid_t cur_fg_pid; 
extern std::string cur_fg_cmd;
extern int cur_fg_jid;
extern char* prev_path;
extern char* cur_path;
class job{
public:
	pid_t pid;
	std::string cmd;
	bool is_stopped;
	time_t entered_time; // the time the job entered the list
	job();
	job(pid_t pidx,std::string cmdx,bool is_stoppedx,time_t entered_timex);
	job(const job& new_job);
	job& operator=(const job& new_job);
	~job();
};
//int ExeComp(char* lineSize);
void update_jobs_list();
int BgCmd(std::string args[MAXARGS], int args_count, std::string command);
int ExeCmd(std::string args[MAXARGS], int args_count, std::string command);
int ExeExternal(std::string args[MAXARGS], int args_count, std::string command);
bool check_if_built_in_cmd(std::string command);
bool fg_empty();
void fg_clean();
void fg_replace(pid_t pidx, std::string cmdx, int jidx = -1);
int break_cmd_to_args(std::string input, std::string(&args)[MAXARGS], std::string delim = " \t\n");
bool insert_job(pid_t pID, std::string cmd, bool is_stopped = false, int job_id = -1);
int search_job(std::string& arg);
bool is_number(std::string& str);
int find_stopped_job();
bool is_fg_have_job_id();
#endif

