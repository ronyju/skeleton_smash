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
    std::string _original_cmd_line;
    char *_args[COMMAND_MAX_ARGS];
    int number_of_args;
public:
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

    bool IsBashCommand(); //RONY
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

        JobEntry(Command *command, unsigned int job_id, bool is_stopped);

        ~JobEntry();

        void StopJobEntry();

        void ReactivateJobEntry();

        bool isStoppedJob();

        void print(); //TODO: Oren - stopped or not
    };

public:
    std::vector<JobEntry *> _vector_all_jobs;

    unsigned int _list_next_job_number = 1; // will be increased every time a job is created
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList(); //TODO : Oren (check if delete)
    void killAllJobs();

    void removeFinishedJobs(); //TODO: Oren -wait
    JobEntry *getJobById(int jobId); //return null when not found
    void removeJobById(int jobId); // wait, is this to kill or just to remove?
    JobEntry *getLastJob(int *lastJobId); //TODO: Oren - wait
    JobEntry *getLastStoppedJob(int *jobId); //TODO: RONY - wait
    bool isInTheBackground(JobEntry *job);
};

//jobs
class JobsCommand : public BuiltInCommand {
    // TODO: Add your data member
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

//fg
class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
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
    std::string GetPrompt(); // TODO:
    void SetPrompt(const char *); //TODO:
};

#endif //SMASH_COMMAND_H_
