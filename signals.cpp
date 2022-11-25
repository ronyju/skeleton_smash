#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    perror("smash: got ctrl-Z");
    if (smash.currentPidInFg) { //there is a process that is running in the foreground

    }

}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation

}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

