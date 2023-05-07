#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
#include <fstream>

namespace logging {

class Log
{
    public:
    enum logType {
        LOG_NONE = 0,
        LOG_ERROR = (1 << 0),
        LOG_WARN = (1 << 1),
        LOG_INFO = (1 << 2),
        LOG_DEBUG = (1 << 3),
    };


    Log();
    ~Log();

    bool open(std::string &filename);

    void setLogType(Log::logType t);
    void clearLogType(Log::logType t);
    bool isTypeEnabled(Log::logType t);
    void flush();
    Log &getCategoryLogger(Log::logType t);
    std::string getCategoryPrefix();

    Log &operator<<(int i);
    Log &operator<<(long unsigned int i);
    Log &operator<<(char c);
    Log &operator<<(float f);
    Log &operator<<(double d);
    Log &operator<<(const char *c);
    Log &operator<<(std::string s);
    Log &operator<<(void *v);

    typedef Log &Manipulator(Log &l);
    Log &operator<<(Manipulator *manip);

    private:
    std::ostream *m_out;
    std::ofstream m_fp;
    int m_types;
    Log::logType m_category;
};

Log &endl(Log &l);
Log &lprefix(Log &l);

#define LOG(c) log.getCategoryLogger(c)


} //namespace logging


extern logging::Log log;

#endif



