#include "log.h"
#include "panel.h"
#include "action.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace logging;
using namespace ad2usb;

//options
static bool debug = false;
static bool daemonMode = false;
static std::string logfile = "-";
static std::string actionpath = "./";
static std::string pidpath = "/var/run/alarm.pid";
static std::string devicepath = "/dev/ttyUSB0";
static Panel *g_panel = NULL;

void handler(int sig)
{
    static int signalCount = 0;
    LOG(Log::LOG_INFO) << lprefix << "SIGINT caught. Shutting down." << endl;
    if (g_panel) {
        g_panel->stopMonitoring();
    }

    signalCount++;
    if (signalCount == 3) {
        exit(0);
    }
}

void print_help()
{
    std::cout << "alarm [options]" << std::endl;
    std::cout << "Options: [-name: description (default)]" << std::endl;
    std::cout << "\t-daemon: Run in daemon mode" << std::endl;
    std::cout << "\t-device: Specify the device path (/dev/ttyUSB0)" << std::endl;
    std::cout << "\t-debug: Turn on debug logging" << std::endl;
    std::cout << "\t-t: Path where action scripts reside (./)" << std::endl;
    std::cout << "\t-pid: Pid path for daemon mode (/var/run/alarm.pid)" << std::endl;
    std::cout << "\t-l -log: Specify the log filename. Use '-' to print to stdout (-)" << std::endl;
    std::cout << "\t-?: This help message" << std::endl;
}

bool writepid()
{
    std::ofstream myfile;
    myfile.open(pidpath.c_str());
    if (!myfile.is_open()) {
        return false;
    }
    myfile << getpid();
    myfile.close();
    return true;
}

bool argparse(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        std::string sarg(argv[i]);
        if (sarg.compare("-debug") == 0) {
            debug = true;
        } else if (sarg.compare("-daemon") == 0) {
            daemonMode = true;
        } else if (sarg.compare("-log") == 0 || sarg.compare("-l") == 0) {
            if (i == argc - 1) {
                std::cout << "Need argument for " << sarg << " option" << std::endl;
                return false;
            }
            logfile = std::string(argv[++i]);
        } else if (sarg.compare("-device") == 0) {
            if (i == argc - 1) {
                std::cout << "Need argument for " << sarg << " option" << std::endl;
                return false;
            }
            devicepath = std::string(argv[++i]);
        } else if (sarg.compare("-?") == 0) {
            print_help();
            return false;
        } else if (sarg.compare("-t") == 0) {
            if (i == argc - 1) {
                std::cout << "Need argument for " << sarg << " option" << std::endl;
                return false;
            }
            actionpath = std::string(argv[++i]);
        } else if (sarg.compare("-pid") == 0) {
            if (i == argc - 1) {
                std::cout << "Need argument for " << sarg << " option" << std::endl;
                return false;
            }
            pidpath = std::string(argv[++i]);
        } else {
            print_help();
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        fprintf(stderr, "Signal handler setup failed\n");
        return -1;
    }
    
    //TODO: setup signal handler to exit gracefully and remove pidpath

    //parse command line args
    if (!argparse(argc, argv)) {
        return 2;
    }

    //open a log file
    if (!log.open(logfile)) {
        fprintf(stderr, "Opening log file %s failed\n", logfile.c_str());
        return -1;
    }

    //start daemon mode if requested
    if (daemonMode) {
        pid = fork();
        if (pid == -1) {
            //error!
            fprintf(stderr, "Error forking daemon process!!\n");
            return -1;
        } else if (pid > 0) {
            return 0;
        }
        //child continues execution
        
        if (!writepid()) {
            return -1;
        }
    }

    if (debug) {
        log.setLogType(Log::LOG_DEBUG);
    }

    LOG(Log::LOG_INFO) << lprefix << "Alarm monitor starting" << endl;
    
    //initialize the components
    Panel panel(devicepath.c_str());
    g_panel = &panel;
    
    //add actions for state changes
    std::stringstream ss;
    ss << actionpath << "state.sh";
    LOG(Log::LOG_DEBUG) << lprefix << "Adding action " << ss.str() << endl;
    Action st(ss.str());
    panel.addAction(Panel::EVENT_STATE, &st);
    
    ss.str(std::string());
    ss << actionpath << "power.sh";
    LOG(Log::LOG_DEBUG) << lprefix << "Adding action " << ss.str() << endl;
    Action pwr(ss.str());
    panel.addAction(Panel::EVENT_POWER, &pwr);

    panel.startMonitoring();

    LOG(Log::LOG_INFO) << lprefix << "Alarm monitor exiting" << endl;

    exit(0);
}
