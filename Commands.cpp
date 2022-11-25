#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <ctime>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

void removeDeadJobs();

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#define CD_TO_OLD_PWD "-"
#define EQUALS 0
#define OLDPWD_NOT_SET NULL
#define SIGKILL 9
#define PROCESS_EXISTS 0
#endif

bool is_process_exist(unsigned int process_pid) {
    int is_still_alive = waitpid(process_pid, nullptr, WNOHANG);
    if (is_still_alive != 0) { return false; }
    return true;
}

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//-----------------JobEntry-------------------

JobsList::JobEntry::JobEntry(Command *command, unsigned int job_id, unsigned int pid, bool is_stopped)
        : _job_id(job_id), _command(command), _is_stopped(is_stopped), _pid(pid) {
    _job_inserted_time = time(NULL); // why is it always 0 ??
}

void JobsList::JobEntry::StopJobEntry() {
    _is_stopped = true;
}

void JobsList::JobEntry::ReactivateJobEntry() {
    _is_stopped = false;
}

void JobsList::JobEntry::print() {
    double seconds_elapsed = difftime(time(NULL), _job_inserted_time);
    if (_is_stopped) {
        std::cout << "[" << _job_id << "]" << _command << " : " << _pid << " " << seconds_elapsed << " secs"
                  << " (stopped)\n";
    } else {
        std::cout << "[" << _job_id << "]" << _command << " : " << _pid << " " << seconds_elapsed << " secs\n";
    }
}
//---------------------------------------------


//-----------------JobsList-------------------

void JobsList::addJob(Command *cmd, unsigned int pid, bool isStopped) {
    removeFinishedJobs();
    _list_max_job_number++;
    JobEntry *new_job = new JobEntry(cmd, _list_max_job_number, pid, isStopped);
    _vector_all_jobs.push_back(new_job);
}

void JobsList::killAllJobs() {
    for (auto &job: _vector_all_jobs) {
        //    removeJobById(job->_job_id); TODO: add this line after implement removeJobById.
        kill(job->_pid, SIGKILL);
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto &job: _vector_all_jobs) {
        if (job->_job_id == jobId) {
            return job;
        }
    }
    return NULL; // not found any job with this id
}

void JobsList::removeFinishedJobs() {
    // this should remove all jobs that were done by now from the vector
    // and update the new highest number in the
    vector<JobEntry *>::iterator it = _vector_all_jobs.begin();
    for (auto job: _vector_all_jobs) {
        if (!(is_process_exist(job->_pid))) {
            _vector_all_jobs.erase(it);
        }
        it++;
    }
    unsigned int current_bigger_job_id = 0;
    for (auto job: _vector_all_jobs) {
        if (job->_job_id > current_bigger_job_id) {
            current_bigger_job_id = job->_job_id;
        }
    }
    if (_list_max_job_number > current_bigger_job_id) {
        _list_max_job_number = current_bigger_job_id;
    }
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto job: _vector_all_jobs) {
        job->print();
    }
}

bool isInTheBackground(JobsList::JobEntry *job) {
    return job->_command->is_background_command;
}
//---------------------------------------------

//-----------------Command-------------------

Command::Command(const char *cmd_line) {
    is_background_command = _isBackgroundComamnd(cmd_line);
    string cmd_trimmed = _trim(string(cmd_line));
    _original_cmd_line = cmd_trimmed;
    _removeBackgroundSign(const_cast<char *>(cmd_trimmed.c_str())); // TODO: check this !!
    number_of_args = _parseCommandLine(const_cast<char *>(cmd_trimmed.c_str()), _args);
    _cmd_line = cmd_trimmed;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}


//chprompt
ChangePromptCommand::ChangePromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangePromptCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    number_of_args < 2 ? smash.SetPrompt("smash") : smash.SetPrompt(_args[1]);

}


//pwd
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char *pwd = getcwd(NULL, 0);
    std::cout << (pwd) << "\n";
}
//showpid

ShowPidCommand::ShowPidCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    int pid = getpid();
    std::cout << (pid) << "\n";
}

//cd
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    _old_pwd = plastPwd;
}

void ChangeDirCommand::execute() {
    if (number_of_args > 2) {
        perror("smash error: cd: too many arguments");
        return;
    }
    char *path = _args[1]; // = second word in the command is the path
    if (strcmp(path, CD_TO_OLD_PWD) == EQUALS) {
        if (*_old_pwd == OLDPWD_NOT_SET) {
            perror("smash error: cd: OLDPWD not set");
            return;
        } else {
            path = *_old_pwd;
        }
    }
    *_old_pwd = getcwd(NULL, 0); // update the old_pwd to the current path
    chdir(path);
}
//---------------------------------------------

SmallShell::SmallShell() {
    _jobs_list = new JobsList();
}

SmallShell::~SmallShell() {
    delete _jobs_list;
}

/*
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if ((firstWord.compare("pwd") == EQUALS) || (firstWord.compare("pwd&") == EQUALS)) {
        return new GetCurrDirCommand(cmd_line);
    } else if ((firstWord.compare("showpid") == EQUALS) || (firstWord.compare("showpid&") == EQUALS)) {
        return new ShowPidCommand(cmd_line);
    } else if ((firstWord.compare("chprompt") == EQUALS) || (firstWord.compare("chprompt&") == EQUALS)) {
        return new ChangePromptCommand(cmd_line);
    } else if ((firstWord.compare("cd") == EQUALS) || (firstWord.compare("cd&") == EQUALS)) {
        return new ChangeDirCommand(cmd_line, &this->_old_pwd);
    } else if ((firstWord.compare("jobs") == EQUALS) || (firstWord.compare("jobs&") == EQUALS)) {
        return new JobsCommand(cmd_line, this->_jobs_list);
    } else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

std::string SmallShell::GetPrompt() {
    return shell_prompt;
}

void SmallShell::SetPrompt(const char *newPromptName) {
    shell_prompt = newPromptName;
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
    is_bash_problem = IsBashCommand();
}

void ExternalCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int son_pid = fork();
    if (son_pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (son_pid == 0 /*son*/) {
        setpgrp();
        if (is_bash_problem) {
            execl("/bin/bash", "bin/bash", "-c", _cmd_line.c_str(), NULL);
        } else {
            execl("/bin/bash", "bin/bash", "-c", _cmd_line.c_str(), NULL); //TODO: all bash?
        }
    }
    if (is_background_command) {
        smash._jobs_list->addJob(this, son_pid);
    } else {
        if (waitpid(son_pid, NULL, WUNTRACED) < 0) {
            perror("smash error: waitpid failed");
        }
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _job_list = jobs;
}

void JobsCommand::execute() {
    _job_list->printJobsList();
}
