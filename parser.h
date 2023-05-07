#include <string>

#ifndef _PARSER_H
#define _PARSER_H

namespace ad2usb {
    
class Parser
{
    public:
    enum alarmState {
        STATE_UNINITIALIZED = 0,
        STATE_READY,
        STATE_ARMED_STAY,
        STATE_ARMED_AWAY,
        STATE_ARMED_INSTANT,
        STATE_ARMED_MAX,
        STATE_ALARM,
        STATE_NOTREADY,
    };
    
    struct alarmStatus {
        //synthesized status
        alarmState state;
        
        //raw status
        bool ready;
        bool arm_away;
        bool arm_stay;
        bool backlight;
        bool prog_mode;
        int numbeep;
        bool bypass;
        bool acpower;
        bool chime;
        bool alarm;
        bool bell;
        bool batlow;
        bool no_delay;
        bool fire;
        bool sys_issue;
        bool permimeter;

        char msg[100];
        
        //TODO: not used yet
        //int numpads;
        //int keypads[10];
    };
    
    Parser();
    ~Parser();
    bool parseRawData(std::string &raw, Parser::alarmStatus *status);
    static std::string stateToString(Parser::alarmState state);
    
    private:
    
};
    
}

#endif
