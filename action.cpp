#include "action.h"
#include "log.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

using namespace logging;

ad2usb::Action::Action(std::string f)
{
    m_filepath = f;
    m_func = NULL;
}

ad2usb::Action::Action(actionFunc *func)
{
    m_func = func;
}

ad2usb::Action::~Action()
{
}

bool ad2usb::Action::execute(int value, std::string &msg)
{
    pid_t pid;
    int status;
    char buf[50];

    if (m_func) {
        return m_func(value, msg);
    } else {
        pid = fork();
        if (pid == 0) {
            //child
            snprintf(buf, sizeof(buf), "%d", value);
            execl(m_filepath.c_str(), m_filepath.c_str(), buf, msg.c_str(), (char *)NULL);
            exit(1);
        } else if (pid == -1) {
            //failure to fork
             log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Failure to fork child process for action " << m_filepath << endl;
             return false;
        }
        //parent
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) {
            log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Child did not exit gracefully for action " << m_filepath << endl;
            return false;
        } else if (WEXITSTATUS(status) != 0) {
            log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Child did not exit with 0 exit code for action " << m_filepath << ": " << WEXITSTATUS(status) << endl;
            return false;
        }
        return true;

    }
}