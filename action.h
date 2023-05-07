#ifndef _ACTION_H
#define _ACTION_H

#include <string>

namespace ad2usb
{

class Action
{
    public:
    typedef bool actionFunc(int value, std::string &msg);
    bool alerted;

    Action(std::string path);
    Action(actionFunc *func);
    ~Action();
    bool execute(int value, std::string &msg);

    private:
    std::string m_filepath;
    actionFunc *m_func;
};

}

#endif
