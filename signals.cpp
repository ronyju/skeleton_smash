#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z" << std::endl;
    pid_t FgPid = smash.currentPidInFg;
    if (FgPid) { //there is a process that is running in the foreground
        smash._jobs_list->addJob(smash.cmd, FgPid, true); //add fg command to jobs
        if (kill(FgPid, SIGSTOP) == 0) {
            std::cout << "smash:process " << FgPid << " was stopped" << std::endl;
        }//send SIGSTOP to the process
        else {
            perror("smash error: kill failed");
        }
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C" << std::endl;
    pid_t FgPid = smash.currentPidInFg;
    if (FgPid) { //there is a process that is running in the foreground
        if (kill(FgPid, SIGKILL) == 0) {
            std::cout << "smash:process " << FgPid << " was killed" << std::endl;
        }//send SIGSTOP to the process
        else {
            perror("smash error: kill failed");
        }
    }
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

