#include "state.h"
#include "log.h"
#include <string.h>

using namespace logging;

namespace {
    bool startsWith(std::string str1, std::string str2)
    {
        if (str1.compare(0, str2.size(), str2) == 0) {
            return true;
        }
        return false;
    }
}

ad2usb::State::State()
{
    memset(&m_status, 0, sizeof(Parser::alarmStatus));
    m_status.state = Parser::STATE_UNINITIALIZED;
}

ad2usb::State::~State()
{
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "State deconstructor" << endl;
}

void ad2usb::State::executeEvent(Panel::alarmEvent event, int value, std::string &msg)
{
    if (m_actions[event]) {
        if (!m_actions[event]->execute(value, msg)) {
            log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "State event action failed state=" <<  (int)event << " value=" << value << endl;
        }
    }
}

void ad2usb::State::setState(Parser::alarmStatus &status)
{
    std::string msg(status.msg);

    if (m_status.state != status.state) {
        //state change
        log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Alarm state change: " <<  Parser::stateToString(status.state) << endl;
        executeEvent(Panel::EVENT_STATE, status.state, msg);
    }
    
    if (m_status.acpower != status.acpower) {
        //power change
        log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Power state change " <<  status.acpower << endl;
        executeEvent(Panel::EVENT_POWER, status.acpower, msg);
    }

    if (m_status.sys_issue != status.sys_issue) {
        //system issue change
        log.getCategoryLogger(Log::LOG_INFO) << lprefix << "System issue change: " <<  status.sys_issue << endl;
        executeEvent(Panel::EVENT_SYS_ISSUE, status.sys_issue, msg);
    }

    if (m_status.state == Parser::STATE_NOTREADY) {
        if (startsWith(std::string(m_status.msg), std::string("FAULT"))) {
            //TODO: store fault message
        }
    }
    
    memcpy(&m_status, &status, sizeof(Parser::alarmStatus));
}

ad2usb::Parser::alarmState ad2usb::State::getAlarmState()
{
    return m_status.state;
}

void ad2usb::State::addAction(Panel::alarmEvent event, Action *a)
{
    m_actions[event] = a;
}

std::string ad2usb::State::getPanelMessage()
{
    std::string pstr(m_status.msg);
    return pstr;
}

