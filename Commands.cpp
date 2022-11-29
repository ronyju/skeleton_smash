#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <ctime>
#include <iomanip>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <regex>


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
#define PERMISSIONS 0655
#define STDOUT_FD 1
#define CHANGE_TO_FATHER ".."
#endif

void computeLPSArray(string pat, int M, int lps[]) {

    // Length of the previous longest
    // prefix suffix
    int len = 0;
    int i = 1;
    lps[0] = 0; // lps[0] is always 0

    // The loop calculates lps[i] for
    // i = 1 to M-1
    while (i < M) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;
            i++;
        } else // (pat[i] != pat[len])
        {

            // This is tricky. Consider the example.
            // AAACAAAA and i = 7. The idea is similar
            // to search step.
            if (len != 0) {
                len = lps[len - 1];

                // Also, note that we do not
                // increment i here
            } else // if (len == 0)
            {
                lps[i] = len;
                i++;
            }
        }
    }
}

int KMPSearch(string pat, string txt) {
    int M = pat.length();
    int N = txt.length();

    // Create lps[] that will hold the longest
    // prefix suffix values for pattern
    int lps[M];
    int j = 0; // index for pat[]

    // Preprocess the pattern (calculate lps[]
    // array)
    computeLPSArray(pat, M, lps);

    int i = 0; // index for txt[]
    int res = 0;
    int next_i = 0;

    while (i < N) {
        if (pat[j] == txt[i]) {
            j++;
            i++;
        }
        if (j == M) {

            // When we find pattern first time,
            // we iterate again to check if there
            // exists more pattern
            j = lps[j - 1];
            res++;
        }

            // Mismatch after j matches
        else if (i < N && pat[j] != txt[i]) {

            // Do not match lps[0..lps[j-1]]
            // characters, they will match anyway
            if (j != 0)
                j = lps[j - 1];
            else
                i = i + 1;
        }
    }
    return res;
}

bool isStringANumber(string suspect) {
    int num = 0;
    while (suspect[num] == '-') { // if it is a negative number
        num++;
    }
    //now, if this is a number, there will be only digits
    for (int i = num; i < suspect.length(); i++) {
        if (isdigit(suspect[i]) == false) { //not a digit
            return false;
        }
    }
    return true;
}

bool is_process_exist(unsigned int process_pid) {
    int is_still_alive = waitpid(process_pid, nullptr, WNOHANG);
    if (is_still_alive != 0) {
        return false;
    }
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
    _job_inserted_time = time(NULL);
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
        std::cout << "[" << _job_id << "] " << _command->_original_cmd_line << " : " << _pid << " " << seconds_elapsed
                  << " secs"
                  << " (stopped)\n";
    } else {
        std::cout << "[" << _job_id << "] " << _command->_original_cmd_line << " : " << _pid << " " << seconds_elapsed
                  << " secs\n";
    }
}

bool JobsList::JobEntry::isStoppedJob() {
    return this->_is_stopped;
}
//---------------------------------------------


//-----------------JobsList-------------------

void JobsList::addJob(Command *cmd, unsigned int pid, bool isStopped) {
    if (getJobByPID(pid) != NULL) { // fg that was stooped. is alredy in the list
        JobEntry *fg_stopped_job = getJobByPID(pid);
        fg_stopped_job->_is_stopped = true;
        //fg_stopped_job->_command->got_fg = false;
        return;
    }
    removeFinishedJobs();
    if (is_process_exist(pid)) {
        JobEntry *new_job;
        _list_max_job_number++;
        cmd->_job_id_if_has = _list_max_job_number;
        new_job = new JobEntry(cmd, _list_max_job_number, pid, isStopped);
        _vector_all_jobs.push_back(new_job);
    }
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
            //TODO: RONY should return?
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

JobsList::JobEntry *JobsList::getJobByPID(unsigned int job_pid) {
    for (auto &job: _vector_all_jobs) {
        if (job->_pid == job_pid) {
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
            it = it - 1; //to resume from same point we stoped before erasing
            _vector_all_jobs.erase(it + 1);
            //TODO: RONY remove the job entery?
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

Command::Command(const char *cmd_line, bool not_allowed_in_background) {
    is_background_command = _isBackgroundComamnd(cmd_line);
    string cmd_trimmed = _trim(string(cmd_line));
    _original_cmd_line = cmd_trimmed;
    _removeBackgroundSign(const_cast<char *>(cmd_trimmed.c_str()));
    number_of_args = _parseCommandLine(const_cast<char *>(cmd_trimmed.c_str()), _args);
    _cmd_line = cmd_trimmed;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line, bool not_allowed_in_background) : Command(cmd_line, true) {}


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
        return;
    }
    std::cout << "smash pid is " << (pid) << "\n";
}

//cd
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    _old_pwd = plastPwd;
}

void
ChangeDirCommand::execute() {
    if (number_of_args > 2) {
        std::cerr << "smash error: cd: too many arguments\n";
        return;
    }
    if (number_of_args == 1) { // just cd does nothing
        return;
    }
    char *path = _args[1]; // = second word in the command is the path
    if (_args[1] ==
        CHANGE_TO_FATHER) { // if cd .. - call bash bin to execute, this was not specified to be implemented by us
        execl("/bin/bash", "bin/bash", "-c", _cmd_line.c_str(), NULL);
    }
    if (strcmp(path, CD_TO_OLD_PWD) == EQUALS) {
        if (*_old_pwd == OLDPWD_NOT_SET) {
            std::cerr << "smash error: cd: OLDPWD not set\n";
            return;
        } else {
            path = *_old_pwd;
        }
    }
    char *temp_new_old_pwd = getcwd(NULL, 0); // update the temp_old_pwd to the current path
    if (chdir(path) == -1) {
        perror("smash error: chdir failed");
        return;
    }
    *_old_pwd = temp_new_old_pwd; //switched to new directory so update old

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
Command *SmallShell::CreateCommand(const char *cmd_line, bool not_allowed_in_background) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    // redirect:
    if (cmd_s.find(">>") != string::npos) {
        return new RedirectionCommand(cmd_line, APPEND);
    } else if (cmd_s.find_first_of('>') != string::npos) {
        return new RedirectionCommand(cmd_line, OVERWRITE);
    }

        // pipes:
    else if (cmd_s.find("|&") != string::npos) {
        return new PipeCommand(cmd_line, STDERR);
    } else if (cmd_s.find_first_of('|') != string::npos) {
        return new PipeCommand(cmd_line, STDOUT);
    }

        // Built in:
    else if ((firstWord.compare("pwd") == EQUALS) || (firstWord.compare("pwd&") == EQUALS)) {
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
    } else if ((firstWord.compare("kill") == EQUALS) || (firstWord.compare("kill&") == EQUALS)) {
        return new KillCommand(cmd_line, this->_jobs_list);
    } else if ((firstWord.compare("fare") == EQUALS) || (firstWord.compare("fare&") == EQUALS)) {
        return new FareCommand(cmd_line);
    }
        // External:
    else {
        return new ExternalCommand(cmd_line, not_allowed_in_background);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line, bool not_allowed_in_background) {
    // TODO: Add your implementation here
    // for example:
    string the_original_command_line = string(cmd_line);
    string cmd_trimmed = _trim(string(cmd_line));
    if (cmd_trimmed.size() == 0) return;
    cmd = CreateCommand(cmd_line, not_allowed_in_background);
    cmd->_original_cmd_line = the_original_command_line;
    if (cmd->error_command_dont_execute != true) { cmd->execute(); }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

std::string SmallShell::GetPrompt() {
    return shell_prompt;
}

void SmallShell::SetPrompt(const char *newPromptName) {
    shell_prompt = newPromptName;
}

ExternalCommand::ExternalCommand(const char *cmd_line, bool not_allowed_in_background) : Command(cmd_line,
                                                                                                 not_allowed_in_background) {
    is_bash_problem = IsComplexCommand();
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
            if (execvp(_args[0], _args) == -1)
                if (kill(getpid(), SIGKILL) == -1) {
                    perror("smash error: kill failed");
                    return;
                }
            perror("smash error: execv failed");
            return;
        }
    }
    if (is_background_command && !_is_not_allowed_in_background) { // if father and bg
        smash._jobs_list->addJob(this, son_pid);
    } else { // if father and not bg
        smash.currentPidInFg = son_pid;
        if (waitpid(son_pid, NULL, WUNTRACED) < 0) {
            perror("smash error: waitpid failed");
            return;
        }
        smash.currentPidInFg = 0;
    }
}

void ExternalCommand::setTimeout(unsigned int seconds_to_timeout) {
    _is_with_timeout = true;
    _seconds_to_timeout = seconds_to_timeout;
}

bool ExternalCommand::IsComplexCommand() {
    return (_cmd_line.find_first_of('*') != string::npos || _cmd_line.find_first_of('?') != string::npos);
}



//-----------------Jobs-------------------

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _job_list = jobs;
}

void JobsCommand::execute() {
    _job_list->printJobsList();
}


ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    jobs->removeFinishedJobs(); //REMOVE
    if ((number_of_args != 1 && number_of_args != 2) || (number_of_args == 2 && !isStringANumber(_args[1]))) {
        std::cerr << "smash error: fg: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    const char *job_id_string;
    _job_list = jobs;
    if (number_of_args == 1) { //default to resume if not specified
        _job_id_to_fg = _job_list->_list_max_job_number;
        _job_entry_to_fg = jobs->getJobById(_job_id_to_fg);
        if (_job_entry_to_fg == NULL) {
            std::cerr << "smash error: fg: jobs list is empty\n";
            error_command_dont_execute = true;
            return;
        }
    } else {
        job_id_string = _args[1];
        _job_id_to_fg = atoi(job_id_string);
        _job_entry_to_fg = jobs->getJobById(_job_id_to_fg);
        if (_job_entry_to_fg == NULL) {
            std::cerr << "smash error: fg: job-id " << job_id_string << " does not exist\n";
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
        return;
    }
    smash.cmd = _job_entry_to_fg->_command;
    smash.currentPidInFg = _job_entry_to_fg->_pid;
    if (waitpid(_job_entry_to_fg->_pid, NULL, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
        return;
    }
    //_job_entry_to_fg->_command->got_fg = true;
    smash.currentPidInFg = 0;
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs, bool called_from_kill) : BuiltInCommand(
        cmd_line) {
    _called_from_kill = called_from_kill;
    if ((number_of_args != 1 && number_of_args != 2) || (number_of_args == 2 && !isStringANumber(_args[1]))) {
        if (!_called_from_kill) { std::cerr << "smash error: bg: invalid arguments\n"; }
        error_command_dont_execute = true;
        return;
    }
    const char *job_id_string;
    _job_list = jobs;
    if (number_of_args == 1) { //default to resume if not specified
        _job_entry_to_bg = _job_list->getLastStoppedJob();
        if (_job_entry_to_bg == NULL) {
            if (!_called_from_kill) { std::cerr << "smash error: bg: there is no stopped jobs to resume\n"; }
            error_command_dont_execute = true;
            return;
        }
        _job_id_to_bg = _job_entry_to_bg->_job_id;
    } else {
        job_id_string = _args[1];
        _job_id_to_bg = atoi(job_id_string);
        _job_entry_to_bg = jobs->getJobById(_job_id_to_bg);
        if (_job_entry_to_bg == NULL) {
            if (!_called_from_kill) { std::cerr << "smash error: bg: job-id " << job_id_string << " does not exist\n"; }
            error_command_dont_execute = true;
            return;
        }
        if (!_job_entry_to_bg->isStoppedJob()) {
            if (!_called_from_kill) {
                std::cerr << "smash error: bg: job-id " << job_id_string << " is already running in the background\n";
            }
            error_command_dont_execute = true;
            return;
        }
    }
}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (error_command_dont_execute) { return; }
    if (!_called_from_kill) {
        std::cout << _job_entry_to_bg->_command->_original_cmd_line << " : " << _job_entry_to_bg->_pid << "\n";
    }
    if (kill(_job_entry_to_bg->_pid, SIGCONT) == -1) {
        perror("“smash error: kill failed”");
        return;
    }
    smash.currentPidInFg = 0; //update that no command is running now in fg
    _job_entry_to_bg->ReactivateJobEntry();
}

//----------------------------------------------------------------------------------
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _job_list = jobs;
}

QuitCommand::~QuitCommand() {}

void QuitCommand::execute() {
    if (number_of_args >= 2) {
        string first_arg(_args[1]);
        if (first_arg == "kill") {
            _job_list->removeFinishedJobs();
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
        return;
    }
//TODO:  RONY delete killed comment printed
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
        first_command = _cmd_line.substr(0, _cmd_line.find_first_of("|&"));
        second_command = _cmd_line.substr(_cmd_line.find_first_of("|&") + 2, _cmd_line.size());
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
        dup2(fd[1], _is_stderr ? 2 : 1);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(first_command.c_str(), true);
        if (kill(getpid(), SIGKILL) == -1) perror("smash error: kill failed");
        return;
    }
    pid_t second_son_pid = fork();
    if (second_son_pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (second_son_pid == 0) { //second son
        SmallShell &smash = SmallShell::getInstance();
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(second_command.c_str(), true);
        if (kill(getpid(), SIGKILL) == -1) perror("smash error: kill failed");
        return;

    }
    close(fd[0]);
    close(fd[1]);
    if (waitpid(first_son_pid, nullptr, 0) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    if (waitpid(second_son_pid, nullptr, 0) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}


bool RedirectionCommand::isFileExists(const char *file_path) {
    int result = open(file_path, 0);
    if (result == -1) {
        perror("“smash error: open failed”");
        return false;
    }
    if (close(result) == -1) { perror("“smash error: close failed”"); };
    return true;
}


// -------------------------- RedirectionCommand ----------------------------------

RedirectionCommand::RedirectionCommand(const char *cmd_line, file_write_approche approche) : Command(cmd_line) {
    _approche = approche;
    if (_approche == OVERWRITE) {
        _command = _cmd_line.substr(0, _cmd_line.find_first_of(">"));
        _file_path = _cmd_line.substr(_cmd_line.find_first_of(">") + 1, _cmd_line.size());
    } else {
        _command = _cmd_line.substr(0, _cmd_line.find_first_of(">>"));
        _file_path = _cmd_line.substr(_cmd_line.find_first_of(">>") + 2, _cmd_line.size());
    }
    _command = _trim(_command);
    _file_path = _trim(_file_path);
}

void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int my_file_fd;
    if (_approche == APPEND) {
        my_file_fd = open(_file_path.c_str(), O_RDWR | O_CREAT | O_APPEND, PERMISSIONS);
    } else { // OVERWRITE
        my_file_fd = open(_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, PERMISSIONS);
    }

    // overwrite stdout with my file, and restore it for later
    if ((my_file_fd) == -1) {
        perror("smash error: open failed");
        return;
    }
    int stdout_restore_fd = dup(STDOUT_FD);
    if (stdout_restore_fd == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (dup2(my_file_fd, STDOUT_FD) == -1) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(my_file_fd) == -1) {
        perror("smash error: close failed");
        return;
    }

    smash.executeCommand(_command.c_str(), true);

    // bring back stdout to its place and close the file
    if (dup2(stdout_restore_fd, STDOUT_FD) == -1) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(stdout_restore_fd) == -1) {
        perror("smash error: close failed");
        return;
    }
}

// -------------------------- Kill Command ----------------------------------


KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    _jobs = jobs;
}

void KillCommand::execute() {
    _jobs->removeFinishedJobs();
    if (number_of_args <= 2 || number_of_args > 3) {
        std::cerr << "smash error: kill: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    if (!((_args[1])[0] == '-')) {
        std::cerr << "smash error: kill: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    if (!isStringANumber(_args[1]) || !isStringANumber(_args[2])) {
        std::cerr << "smash error: kill: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    string prepare_signal = _args[1];
    prepare_signal = prepare_signal.erase(0, 1);
    if (!isStringANumber(prepare_signal)) {
        std::cerr << "smash error: kill: invalid arguments";
        error_command_dont_execute = true;
        return;
    }
    _signal_number = stoi(prepare_signal);
    if (_signal_number < 1 || _signal_number > 30) {
        std::cerr << "smash error: kill: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    string job_id_string = _args[2];
    unsigned int job_id = stoi(job_id_string);
    JobsList::JobEntry *job_entry = _jobs->getJobById(job_id);
    if (job_entry == NULL) {
        std::cerr << "smash error: kill: job-id " + job_id_string + " does not exist\n";
        error_command_dont_execute = true;
        return;
    }

    unsigned int job_pid = job_entry->_pid;

    if (_signal_number == SIGSTOP) { // stop will only get jobs that are already in the list, no need to add a new one
        // add stopped to the jobs list
        if (kill(job_pid, SIGSTOP) == -1) {
            perror("smash error: kill failed\n");
            return;
        } else {
            job_entry->_is_stopped = true;
            cout << "signal number " << _signal_number << " was sent to pid " << job_pid << "\n";
            return;
        }
    }

    if (_signal_number == SIGCONT) {
        if (job_entry->isStoppedJob()) { // if it is not stopped just send the signal.
            if (job_entry->_command->is_background_command) {
                // call bg - it was stopped and need to continue but stay in the background:
                std::string bg_cmd_line = "bg ";
                bg_cmd_line += _args[2];
                Command *bg_command = new BackgroundCommand(bg_cmd_line.c_str(), _jobs, true);
                if (bg_command->error_command_dont_execute != true) { bg_command->execute(); }
                delete bg_command;
                cout << "signal number " << _signal_number << " was sent to pid " << job_pid << "\n";
                return;
            }
        }
    }

    if (_signal_number == SIGKILL) {
        if (kill(job_pid, _signal_number) == -1) {
            perror("“smash error: kill failed”");
            return;
        } else {
            _jobs->removeJobById(
                    job_id);// rony added, I was thinking that the dead job wont appear existed but it is so need to delete my self from the list.
            //unsigned int job_pid = _jobs->getJobByPID(job_pid)->_pid;
            std::cout << "signal number " << _signal_number << " was sent to pid " << job_pid << "\n";
            _jobs->removeFinishedJobs(); // if killed will be removed from jobs
            return;
        }
    }

    if (kill(job_pid, _signal_number) == -1) {
        perror("“smash error: kill failed”");
        return;
    } else {
        //unsigned int job_pid = _jobs->getJobByPID(job_pid)->_pid;
        std::cout << "signal number " << _signal_number << " was sent to pid " << job_pid << "\n";
    }
    _jobs->removeFinishedJobs(); // if killed will be removed from jobs
}

// -------------------------- Fare Command ----------------------------------
FareCommand::FareCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    if (number_of_args != 4) {
        std::cerr << "smash error: fare: invalid arguments\n";
        error_command_dont_execute = true;
        return;
    }
    _file_name = _args[1];
    _find = _args[2];
    _replace = _args[3];
}

bool FareCommand::IsFileExist() {
    int result = open(_file_name.c_str(), 0);
    if (result == -1) {
        perror("“smash error: open failed”");
        return false;
    }
    if (close(result) == -1) { perror("“smash error: close failed”"); };
    return true;
}

void FareCommand::execute() {
    if (error_command_dont_execute) { return; }
    if (!IsFileExist()) {
        return;
    }
    std::ifstream infile(_file_name);
    std::ofstream outfile("temp_file.txt");
    std::string line;
    int counter = 0;
    while (std::getline(infile, line)) {
        string new_line = line;
        new_line = std::regex_replace(new_line, std::regex(_find), _replace);
        counter += KMPSearch(_find, line);
        // write new line to the other file
        outfile << new_line;
        outfile << endl;
    }
    infile.close();
    outfile.close();

    remove(_file_name.c_str());
    rename("temp_file.txt", _file_name.c_str());
    std::cout << "replaced " << counter << " instances of the string “" << _find << "”\n";
}
