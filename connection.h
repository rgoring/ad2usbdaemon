#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <string>
#include <stdio.h>

namespace ad2usb
{

class Connection
{
    public:

    Connection(const char *device);
    ~Connection();
    bool isDataAvailable();

    bool connect();
    bool disconnect();
    bool reconnect();
    bool getrawoutput(std::string &str);
    void reboot();
    void writeKeypadData(const std::string &data);

    private:
    bool deviceExists();
    std::string m_devPath;
    FILE *m_file;
    char m_buf[4096];
};


} //namespace ad2usb
#endif
