#include "parser.h"
#include "log.h"

#include <string.h>

using namespace logging;

ad2usb::Parser::Parser()
{
}

ad2usb::Parser::~Parser()
{
}

std::string ad2usb::Parser::stateToString(Parser::alarmState state)
{
    switch(state)
    {
        case Parser::STATE_READY:
            return std::string("Ready");
            break;
        case Parser::STATE_ARMED_STAY:
            return std::string("Armed Stay");
            break;
        case Parser::STATE_ARMED_AWAY:
            return std::string("Armed Away");
            break;
        case Parser::STATE_ARMED_INSTANT:
            return std::string("Armed Instant");
            break;
        case Parser::STATE_ARMED_MAX:
            return std::string("Armed Max");
            break;
        case Parser::STATE_ALARM:
            return std::string("Alarm");
            break;
        case Parser::STATE_NOTREADY:
            return std::string("Not Ready");
            break;
        case Parser::STATE_UNINITIALIZED:
            return std::string("Uninitialized");
        default:
            break;
    }
    return std::string("unknown");
}

bool bitToBool(char bit)
{
    if (bit == '1') {
        return true;
    } //interprets '0' and '-' as false
    return false;
}

bool ad2usb::Parser::parseRawData(std::string &raw, Parser::alarmStatus *status)
{
    if (raw[0] != '[') {
        return false;
    }

    if (raw.length() != 94) {
        log.getCategoryLogger(Log::LOG_WARN) << lprefix << "Incomplete message (length=" << raw.length() << "): " << raw << endl;
        return false;
    }
    
    memset(status, 0, sizeof(Parser::alarmStatus));
    
    status->ready = bitToBool(raw[1]);
    status->arm_away = bitToBool(raw[2]);
    status->arm_stay = bitToBool(raw[3]);
    status->backlight = bitToBool(raw[4]);
    status->prog_mode = bitToBool(raw[5]);
    status->numbeep = 0; //raw[6] as int
    status->bypass = bitToBool(raw[7]);
    status->acpower = bitToBool(raw[8]);
    status->chime = bitToBool(raw[9]);
    status->alarm = bitToBool(raw[10]);
    status->bell = bitToBool(raw[11]);
    status->batlow = bitToBool(raw[12]);
    //anything below here does not show on AD2USB v2.0 :-(
    //Can use the message string to determine no delay
    status->no_delay = bitToBool(raw[13]);
    status->fire = bitToBool(raw[14]);
    status->sys_issue = bitToBool(raw[15]);
    status->permimeter = bitToBool(raw[16]);

    memcpy(status->msg, raw.c_str()+61, 32);

    //assume message contains no delay info for ad2usb v2.0
    if ((raw.find("*INSTANT*") != std::string::npos) || (raw.find("*MAXIMUM*") != std::string::npos)) {
        status->no_delay = true;
    }

    if (status->ready && !status->alarm) {
        status->state = Parser::STATE_READY;
    } else if (status->arm_stay && !status->no_delay && !status->alarm) {
        status->state = Parser::STATE_ARMED_STAY;
    } else if (status->arm_stay && status->no_delay && !status->alarm) {
        status->state = Parser::STATE_ARMED_INSTANT;
    } else if (status->arm_away && !status->no_delay && !status->alarm) {
        status->state = Parser::STATE_ARMED_AWAY;
    } else if (status->arm_away && status->no_delay && !status->alarm) {
        status->state = Parser::STATE_ARMED_MAX;
    } else if (status->alarm) {
        status->state = Parser::STATE_ALARM;
    } else {
        status->state = Parser::STATE_NOTREADY;
    }

    return true;
}
