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

bool IsArgumentANumer(char *arg) {
    return true;// TODO: check if the input contains latter's
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

bool JobsList::JobEntry::isStoppedJob() {
    return this->_is_stopped;
}
//---------------------------------------------


//-----------------JobsList-------------------

void JobsList::addJob(Command *cmd, unsigned int pid, bool isStopped) {
    removeFinishedJobs();
    _list_max_job_number++;
    JobEntry *new_job = new JobEntry(cmd, _list_max_job_number, pid, isStopped);
    _vector_all_jobs.push_back(new_job);
}

void JobsList::removeJobById(int jobId) {
    vector<JobEntry *>::iterator it = _vector_all_jobs.begin();
    for (auto job: _vector_all_jobs) {
        if (job->_job_id == jobId) {
            _vector_all_jobs.erase(it);
        }
        it++;
    }
    UpdateMaxJob();
}

void JobsList::killAllJobs() {
    for (auto &job: _vector_all_jobs) {
        removeJobById(job->_job_id);
        if (kill(job->_pid, SIGKILL) == -1) {
            perror("“smash error: kill failed”");
        }

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

void JobsList::UpdateMaxJob() {
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
    UpdateMaxJob();
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto job: _vector_all_jobs) {
        job->print();
    }
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
    unsigned int highest_job_num_stooped = 0;
    for (auto job: _vector_all_jobs) {
        if (job->isStoppedJob()) {
            if (highest_job_num_stooped < job->_job_id) {
                highest_job_num_stooped = job->_job_id;
            }
        }
    }
    return getJobById(highest_job_num_stooped);
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
    if (pid == -1) {
        perror("“smash error: getpid failed”");
    }
    std::cout << (pid) << "\n";
}

//cd
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    _old_pwd = plastPwd;
}

void
ChangeDirCommand::execute() { //TODO: if cd gets no argument it needs to ignore? now it dose Segmentation fault (core dumped) and exit smash
    if (number_of_args > 2) {
        std::cout << "smash error: cd: too many arguments\n";
        return;
    }
    char *path = _args[1]; // = second word in the command is the path
    if (strcmp(path, CD_TO_OLD_PWD) == EQUALS) {
        if (*_old_pwd == OLDPWD_NOT_SET) {
            std::cout << "smash error: cd: OLDPWD not set\n";
            return;
        } else {
            path = *_old_pwd;
        }
    }
    *_old_pwd = getcwd(NULL, 0); // update the old_pwd to the current path
    if (chdir(path) == -1) {
        perror("smash error: chdir failed");
    }
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
    } else if ((firstWord.compare("fg") == EQUALS) || (firstWord.compare("fg&") == EQUALS)) {
        return new ForegroundCommand(cmd_line, this->_jobs_list);
    } else if ((firstWord.compare("bg") == EQUALS) || (firstWord.compare("bg&") == EQUALS)) {
        return new BackgroundCommand(cmd_line, this->_jobs_list);
    } else if ((firstWord.compare("quit") == EQUALS) || (firstWord.compare("quit&") == EQUALS)) {
        return new QuitCommand(cmd_line, this->_jobs_list);
    } else if (cmd_s.find("|&") != string::npos) {
        return new PipeCommand(cmd_line, STDERR);
    } else if (cmd_s.find_first_of('|') != string::npos) {
        return new PipeCommand(cmd_line, STDOUT);
    } else if (cmd_s.find(">>") != string::npos) {
        //return new RedirectionCommand(cmd_line, APPEND);
    } else if (cmd_s.find_first_of('>') != string::npos) {
        //return new RedirectionCommand(cmd_line, DONT_APPEND);
    } else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    cmd = CreateCommand(cmd_line);
    if (cmd->error_command_dont_execute != true) { cmd->execute(); }
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
    pid_t son_pid = fork();
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
        smash.currentPidInFg = son_pid;
        if (waitpid(son_pid, NULL, WUNTRACED) < 0) {
            perror("smash error: waitpid failed");
        }
        smash.currentPidInFg = 0;
    }
}

//-----------------Jobs-------------------

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _job_list = jobs;
}

void JobsCommand::execute() {
    _job_list->printJobsList();
}


ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    if ((number_of_args != 1 && number_of_args != 2) || (number_of_args == 2 && !IsArgumentANumer(_args[1]))) {
        std::cout << "smash error: fg: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    const char *job_id_string;
    _job_list = jobs;
    if (number_of_args == 1) { //default to resume if not specified
        _job_id_to_fg = _job_list->_list_max_job_number;
        _job_entry_to_fg = jobs->getJobById(_job_id_to_fg);
        if (_job_entry_to_fg == NULL) {
            cout << "smash error: fg: jobs list is empty\n";
            error_command_dont_execute = true;
            return;
        }
    } else {
        job_id_string = _args[1];
        _job_id_to_fg = atoi(job_id_string);
        _job_entry_to_fg = jobs->getJobById(_job_id_to_fg);
        if (_job_entry_to_fg == NULL) {
            std::cout << "smash error: fg: job-id " << job_id_string << " does not exist\n";
            error_command_dont_execute = true;
            return;
        }
    }
}

void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (error_command_dont_execute) { return; }
    std::cout << _job_entry_to_fg->_command->_original_cmd_line << " : " << _job_entry_to_fg->_pid << "\n";
    if (kill(_job_entry_to_fg->_pid, SIGCONT) == -1) {
        perror("“smash error: kill failed”");
    }
    smash.cmd = _job_entry_to_fg->_command;
    smash.currentPidInFg = _job_entry_to_fg->_pid;
    if (waitpid(_job_entry_to_fg->_pid, NULL, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
    }
    _job_list->removeJobById(_job_id_to_fg);
    smash.currentPidInFg = 0;
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    if ((number_of_args != 1 && number_of_args != 2) || (number_of_args == 2 && !IsArgumentANumer(_args[1]))) {
        std::cout << "smash error: bg: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    const char *job_id_string;
    _job_list = jobs;
    if (number_of_args == 1) { //default to resume if not specified
        _job_entry_to_bg = _job_list->getLastStoppedJob();
        if (_job_entry_to_bg == NULL) {
            cout << "smash error: bg: there is no stopped jobs to resume\n";
            error_command_dont_execute = true;
            return;
        }
        _job_id_to_bg = _job_entry_to_bg->_job_id;
    } else {
        job_id_string = _args[1];
        _job_id_to_bg = atoi(job_id_string);
        _job_entry_to_bg = jobs->getJobById(_job_id_to_bg);
        if (_job_entry_to_bg == NULL) {
            std::cout << "smash error: bg: job-id " << job_id_string << " does not exist\n";
            error_command_dont_execute = true;
            return;
        }
        if (!_job_entry_to_bg->isStoppedJob()) {
            std::cout << "smash error: bg: job-id " << job_id_string << " is already running in the background\n";
            error_command_dont_execute = true;
            return;
        }
    }
}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (error_command_dont_execute) { return; }
    std::cout << _job_entry_to_bg->_command->_original_cmd_line << " : " << _job_entry_to_bg->_pid << "\n";
    if (kill(_job_entry_to_bg->_pid, SIGCONT) == -1) {
        perror("“smash error: kill failed”");
    }
    smash.currentPidInFg = 0; //update that no command is running now in fg
    _job_entry_to_bg->ReactivateJobEntry();
}

//----------------------------------------
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _job_list = jobs;
}

QuitCommand::~QuitCommand() {}

void QuitCommand::execute() {
    if (number_of_args >= 2) {
        string first_arg(_args[1]);
        if (first_arg == "kill") {
            std::cout << "smash: sending SIGKILL signal to " << _job_list->_vector_all_jobs.size() << " jobs:"
                      << std::endl;
            for (auto job: _job_list->_vector_all_jobs) {
                std::cout << job->_pid << ": " << job->_command->_original_cmd_line << endl;
            }
            _job_list->killAllJobs();
        }
    }
    if (kill(getpid(), SIGKILL) == 0) {
        perror("smash error: kill failed");
    }
//TODO: delete killed comment printed
}

PipeCommand::PipeCommand(const char *cmd_line, bool is_stderr) : Command(cmd_line), _is_stderr(is_stderr) {}

void PipeCommand::execute() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }
    string first_command;
    string second_command;
    if (_is_stderr) {
        first_command = _cmd_line.substr(0, _cmd_line.find_first_of('|&'));
        second_command = _cmd_line.substr(_cmd_line.find_first_of('|&') + 2, _cmd_line.size());
        first_command = _trim(first_command);
        second_command = _trim(second_command);
    } else {
        first_command = _cmd_line.substr(0, _cmd_line.find_first_of('|'));
        second_command = _cmd_line.substr(_cmd_line.find_first_of('|') + 1, _cmd_line.size());
        first_command = _trim(first_command);
        second_command = _trim(second_command);
    }
    pid_t first_son_pid = fork();
    if (first_son_pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (first_son_pid == 0) { //first son
        SmallShell &smash = SmallShell::getInstance();
        dup2(fd[1], _is_stderr ? 2 : 1); //TODO: add err option dup2(fd[1], 2);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(first_command.c_str());
        if (kill(getpid(), SIGKILL) == -1) perror("smash error: kill failed");
        return;
    }
    pid_t second_son_pid = fork();
    if (second_son_pid == -1) {
        perror("smash error: fork failed");
    }
    if (second_son_pid == 0) { //second son
        SmallShell &smash = SmallShell::getInstance();
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(second_command.c_str());
        if (kill(getpid(), SIGKILL) == -1) perror("smash error: kill failed");
        return;

    }
    close(fd[0]);
    close(fd[1]);
    if (waitpid(first_son_pid, nullptr, 0) == -1) perror("smash error: waitpid failed");
    if (waitpid(second_son_pid, nullptr, 0) == -1) perror("smash error: waitpid failed");
}



