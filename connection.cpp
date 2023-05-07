#include "log.h"
#include "connection.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace logging;

ad2usb::Connection::Connection(const char *device)
{
    m_devPath = std::string(device);
    m_file = NULL;
}

ad2usb::Connection::~Connection()
{
    disconnect();
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Connection deconstructor" << endl;
}

void ad2usb::Connection::reboot()
{
    fputs("=", m_file);
}

bool ad2usb::Connection::reconnect()
{
    if (!disconnect()) {
        return false;
    }
    if (!connect()) {
        return false;
    }
    return true;
}

bool ad2usb::Connection::isDataAvailable()
{
    struct timeval timeout; // timeout param
    bool flag = false;
    int fd;

    if (!deviceExists()) {
        return false;
    }

    if (!m_file) {
        return false;
    }
    fd = fileno(m_file);

    fd_set fdvar;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdvar);
    FD_SET(fd, &fdvar);
    select(fd+1, (fd_set *)&fdvar, (fd_set *)0, (fd_set *)0, &timeout);
    if(FD_ISSET(fd, &fdvar)) {
        flag = true;
    }
    return flag;
}

//void ad2usb::Connection::requestFaults()
//{
//    char buf[10];
//    snprintf(buf, sizeof(buf), "*");
//    log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Getting Faults " << buf << endl;
//    int fd = open(m_devPath.c_str(), O_WRONLY | O_TRUNC);
//    write(fd, buf, strlen(buf));
//    close(fd);
//}

void ad2usb::Connection::writeKeypadData(const std::string &data)
{
    log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Writing keypad data " << data << endl;
    int fd = open(m_devPath.c_str(), O_WRONLY | O_TRUNC);
    write(fd, data.c_str(), data.length());
    close(fd);
}

bool ad2usb::Connection::connect()
{
    pid_t pid;
    int status;

    if (!deviceExists()) {
        return false;
    }

    pid = fork();
    if (pid == 0) {
        //child
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Child executing stty" << endl;
        execl("/bin/stty", "stty", "-F", m_devPath.c_str(), "9600", "-parenb", "cs8", "-cstopb", "-ixon", "-icanon", "min", "1", "time", "1", "-echo", "-echoe", "-echok", "-echoctl", "-echoke", "noflsh", (char *)NULL);
        exit(1);
    } else if (pid == -1) {
        //failure to fork
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Failure to fork child process for stty" << endl;
        return false;
    }
    //parent
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Parent waiting on pid " << (int)pid << endl;
    waitpid(pid, &status, 0);
    //check child exit status
    if (!WIFEXITED(status)) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "stty child did not exit gracefully" << endl;
        return false;
    } else if (WEXITSTATUS(status) != 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Child did not exit with 0 exit code for stty: " << WEXITSTATUS(status) << endl;
        return false;
    }
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Parent done waiting." << endl;

    if ((m_file = fopen(m_devPath.c_str(), "r")) == NULL) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Failure to open device " << m_devPath << endl;
        return false;
    }

    log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Successfully opened device " << m_devPath << endl;

    return true;
}

bool ad2usb::Connection::disconnect()
{
    if (m_file) {
        fclose(m_file);
        m_file = NULL;
    }
    return true;
}

bool ad2usb::Connection::getrawoutput(std::string &str)
{
    bool done = false;
    int i;
    int totalBytes = 0;

    if (!deviceExists()) {
        return false;
    }

    if (!m_file) {
        return false;
    }

    memset(m_buf, '\0', sizeof(m_buf));

    do {
        char c = fgetc(m_file);
        m_buf[totalBytes] = c;
        if (c == '\n') {
            m_buf[totalBytes] = '\0';
            done = true;
        }
        totalBytes++;

        //safety check
        if (totalBytes == sizeof(m_buf)) {
            done = true;
        }
    } while (!done);

    if (totalBytes == 1) {
        return false;
    }

    str = std::string(m_buf);

    return true;
}

bool ad2usb::Connection::deviceExists()
{
    if (access(m_devPath.c_str(), F_OK) == -1) {
        return false;
    }
    return true;
}
