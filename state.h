#ifndef _STATE_H
#define _STATE_H

#include "parser.h"
#include "action.h"
#include "panel.h"
#include <string>

namespace ad2usb
{
    
class State
{
    public:

    State();
    ~State();
    void setState(Parser::alarmStatus &status);
    ad2usb::Parser::alarmState getAlarmState();
    void addAction(Panel::alarmEvent event, Action *a);
    std::string getPanelMessage();
    
    private:
    void executeEvent(Panel::alarmEvent event, int value, std::string &msg);
    Parser::alarmStatus m_status;
    Action *m_actions[Panel::EVENT_LAST];
};
    
}

#endif
