#include "log.h"
#include <time.h>

logging::Log log;

logging::Log::Log()
{
    m_types = LOG_INFO | LOG_WARN | LOG_ERROR;
    m_category = LOG_NONE;
    m_out = NULL;
}

logging::Log::~Log()
{
    //close file handle
    if (m_out) {
        flush();
    }
}

bool logging::Log::open(std::string &filename)
{
    if (filename.compare("-") == 0) {
        m_out = &std::cout;
    } else {
        //open file handle
        m_fp.open(filename.c_str(), std::ios_base::out | std::ios_base::app);
        if (!m_fp.is_open())
        {
            //do something
            return false;
        }
        m_out = &m_fp;
    }
    return true;
}

void logging::Log::setLogType(logging::Log::logType type)
{
    m_types |= type;
}

void logging::Log::clearLogType(logging::Log::logType type)
{
    m_types &= (~type);
}

std::string logging::Log::getCategoryPrefix()
{
    std::string prefix;

    switch (this->m_category)
    {
    case LOG_ERROR:
        prefix = "ERROR: ";
        break;
    case LOG_WARN:
        prefix = "WARN: ";
        break;
    case LOG_INFO:
        prefix = "INFO: ";
        break;
    case LOG_DEBUG:
        prefix = "DEBUG: ";
        break;
    case LOG_NONE:
    default:
        break;
    }

    return prefix;
}

bool logging::Log::isTypeEnabled(logging::Log::logType type)
{
    return (m_types & type);
}

logging::Log &logging::Log::operator<<(int i)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << i;}
    return *this;
}

logging::Log &logging::Log::operator<<(long unsigned int i)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << i;}
    return *this;
}

logging::Log &logging::Log::operator<<(char c)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << c;}
    return *this;
}

logging::Log &logging::Log::operator<<(std::string s)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << s;}
    return *this;
}

logging::Log &logging::Log::operator<<(const char *c)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << c;}
    return *this;
}

logging::Log &logging::Log::operator<<(float f)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << f;}
    return *this;
}

logging::Log &logging::Log::operator<<(double d)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << d;}
    return *this;
}

logging::Log &logging::Log::operator<<(void *v)
{
    if (this->isTypeEnabled(this->m_category)) {*m_out << v;}
    return *this;
}

logging::Log &logging::Log::operator<<(logging::Log::Manipulator *m)
{
    if (this->isTypeEnabled(this->m_category)) {m(*this);}
    return *this;
}

logging::Log &logging::Log::getCategoryLogger(logging::Log::logType t)
{
    this->m_category = t;
    return *this;
}

void logging::Log::flush()
{
    m_out->flush();
}

//non-class function
logging::Log &logging::endl(logging::Log &l)
{
    l << "\n";
    l.flush();
    return l;
}

logging::Log &logging::lprefix(logging::Log &l)
{
    time_t rawtime;
    struct tm *timeinfo;
    char buf[200];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buf, sizeof(buf), "%m/%d/%Y %H:%M:%S %a", timeinfo);
    l << buf << " " << l.getCategoryPrefix();
    return l;
}

