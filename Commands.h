#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#define OLDPWD_NOT_SET NULL

#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define STDOUT  false
#define STDERR  true

enum file_write_approche {
    APPEND, OVERWRITE
};


class Command {
// TODO: Add your data members
protected:
    unsigned _seconds_to_timeout = 0;
    bool _is_with_timeout = false;
    std::string _cmd_line;
    bool _is_not_allowed_in_background = false;
    char *_args[COMMAND_MAX_ARGS];
    int number_of_args;
    public:
    std::string _original_cmd_line;

    Command(const char *cmd_line, bool not_allowed_in_background = false);

    bool is_background_command = false;

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
    bool error_command_dont_execute = false;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line, bool not_allowed_in_background = false);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line, bool not_allowed_in_background = false);

    virtual ~ExternalCommand() {}

    void execute() override;

    bool IsComplexCommand();

    void setTimeout(unsigned int seconds_to_timeout);

private:
    bool is_bash_problem;
};

class PipeCommand : public Command {
    // TODO: Add your data members
    bool _is_stderr = false;
public:
    PipeCommand(const char *cmd_line, bool is_stderr);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    file_write_approche _approche;
    std::string _command;
    std::string _file_path;
public:
    RedirectionCommand(const char *cmd_line, file_write_approche approche);

    virtual ~RedirectionCommand() {}

    void execute() override;

    bool isFileExists(const char *file_path);

    //void prepare() override;
    //void cleanup() override;
};


//chprompt
class ChangePromptCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangePromptCommand(const char *cmd_line);

    virtual ~ChangePromptCommand() {}

    void execute() override;
};

//cd
class ChangeDirCommand : public BuiltInCommand {
public:
// TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;

private:
    char **_old_pwd = NULL;
};

//pwd
class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

//showpid
class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

//quit
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    JobsList *_job_list;

    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand();

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        unsigned int _job_id;
        unsigned int _pid;
        Command *_command;
        bool _is_stopped;
        time_t _job_inserted_time;

        JobEntry(Command *command, unsigned int job_id, unsigned pid, bool is_stopped);

        ~JobEntry();

        void StopJobEntry();

        void ReactivateJobEntry();

        bool isStoppedJob();

        void print(); //TODO: Oren - stopped or not
    };

public:
    std::vector<JobEntry *> _vector_all_jobs;

    unsigned int _list_max_job_number = 0; // when 0 the vector is empty
    JobsList() {};

    ~JobsList() {};

    void addJob(Command *cmd, unsigned int pid, bool isStopped = false);

    void printJobsList(); //TODO : RONY (check if delete)
    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId); //return null when not found
    JobEntry *getJobByPID(unsigned int job_pid); //return null when not found
    void removeJobById(int jobId);

    JobEntry *getLastStoppedJob();

    JobEntry *getLastJob(int *lastJobId); //TODO: Oren - wait
    bool isInTheBackground(JobEntry *job);

    void UpdateMaxJob();
};

//jobs
class JobsCommand : public BuiltInCommand {
public:
    JobsList *_job_list;

    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

//fg
class ForegroundCommand : public BuiltInCommand {
public:
    JobsList *_job_list;
    JobsList::JobEntry *_job_entry_to_fg;
    unsigned int _job_id_to_fg;

    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

//bg
class BackgroundCommand : public BuiltInCommand {
public:
    JobsList *_job_list;
    JobsList::JobEntry *_job_entry_to_bg;
    unsigned int _job_id_to_bg;
    bool _called_from_kill = false;

    BackgroundCommand(const char *cmd_line, JobsList *jobs, bool called_from_kill = false);

    virtual ~BackgroundCommand() {}

    void execute() override;
};

class AlarmsList {
public:
    class AlarmEntry {
    public:
        unsigned int _pid;
        Command *_command;
        time_t _job_inserted_time;
        unsigned int _sec_until_alarm;

        AlarmEntry(Command *command, unsigned pid, unsigned int sec_until_alarm);

        ~AlarmEntry();

        unsigned int AlarmExpectedTime();
    };

public:
    std::list<AlarmEntry *> _list_all_alarms;

    AlarmsList() {};

    ~AlarmsList() {};

    void addJob(Command *cmd, unsigned int pid, unsigned int sec_until_alarm);

    void PlaceJobInTheList(AlarmEntry *entry, unsigned int expected_time);

    AlarmEntry *GetFirstInList();

    void removeFromList(unsigned int job_id); //TODO: make sure when alarm get stop or killed it is removed from here!


};

/*
//option 7
class TimeoutCommand : public BuiltInCommand
        //TODO: what happenes if a job with alarm got stooped ? for now I will leave it in the list, and mark it stooped to ignore it

// I need to add to Command/External, bool timeout, int timeout_seconds. it will only be relevant to external commands any way
// I need to crete a list of all commands who got time out
// when timeout is set execute timeout - there call the sys call sigaction instead of signal, and use SA_RESTART flag
// add it to the timeout list and (use JobEntey? or command in there) and exute the commend like you did in > />>
// than when the sig commes, go to the list and find who it's belong to (how?) and print and stop it ;
public:
    explicit TimeoutCommand(const char *cmd_line);

    virtual ~TimeoutCommand() {}

    void execute() override;
};
*/

//option 7 TODO: add is_not_allwed_on_backgroung?
class FareCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    FareCommand(const char *cmd_line);

    virtual ~FareCommand() {}

    void execute() override;
};

//option 5 TODO: add is_not_allwed_on_backgroung?
class SetcoreCommand : public BuiltInCommand {
    /* Optional */
    int _new_core;
public:
    SetcoreCommand(const char *cmd_line);

    virtual ~SetcoreCommand() {}

    void execute() override;
};

//bunos 5 TODO: add is_not_allwed_on_backgroung?
class KillCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
public:
    int _signal_number;
    JobsList *_jobs;

    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;

};

class SmallShell {
private:
    // TODO: Add your data members
    std::string shell_prompt = "smash";
public:
    pid_t currentPidInFg = 0;
    Command *cmd;
    JobsList *_jobs_list;
    JobsList *_timeout_jobs_list; // TODO: make sure you manage this right with add and remove
    char *_old_pwd = OLDPWD_NOT_SET; // will be set by cd
    Command *CreateCommand(const char *cmd_line, bool not_allowed_in_background = false);

    SmallShell();

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line, bool not_allowed_in_background = false);

    // TODO: add extra methods as needed
    std::string GetPrompt();

    void SetPrompt(const char *);

};

#endif //SMASH_COMMAND_H_
