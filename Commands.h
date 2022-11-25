#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#define OLDPWD_NOT_SET NULL

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
protected:

    std::string _cmd_line;

    char *_args[COMMAND_MAX_ARGS];
    int number_of_args;
public:
    std::string _original_cmd_line;

    Command(const char *cmd_line);

    bool is_background_command = false;

    virtual ~Command() = default;

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;

    bool IsBashCommand() { return true; } //TODO: change?
private:
    bool is_bash_problem;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
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

    virtual ~QuitCommand() {}

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
    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId); //TODO: Oren - wait
    JobEntry *getLastStoppedJob(int *jobId); //TODO: RONY - wait dont think I need this.
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
    BackgroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~BackgroundCommand() {}

    void execute() override;
};

//option 7
class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
public:
    explicit TimeoutCommand(const char *cmd_line);

    virtual ~TimeoutCommand() {}

    void execute() override;
};

//option 7
class FareCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    FareCommand(const char *cmd_line);

    virtual ~FareCommand() {}

    void execute() override;
};

//option 5
class SetcoreCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    SetcoreCommand(const char *cmd_line);

    virtual ~SetcoreCommand() {}

    void execute() override;
};

//bunos 5
class KillCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
public:
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
    char *_old_pwd = OLDPWD_NOT_SET; // will be set by cd
    Command *CreateCommand(const char *cmd_line);

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

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
    std::string GetPrompt();

    void SetPrompt(const char *);

};

#endif //SMASH_COMMAND_H_
