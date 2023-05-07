#ifndef _PANEL_H
#define _PANEL_H

namespace ad2usb
{

class Connection;
class State;
class UI;
class Parser;
class Action;

class Panel
{
public:
    enum alarmEvent {
        EVENT_STATE = 0,
        EVENT_POWER,
        EVENT_SYS_ISSUE,
        EVENT_LAST //must be last
    };

	Panel(const char *device);
	~Panel();

	ad2usb::Connection *getConnection();
	ad2usb::State *getState();
	void addAction(alarmEvent event, Action *a);

	void startMonitoring();
	void stopMonitoring();

private:
	bool m_monitoring;
	ad2usb::UI *m_ui;
	ad2usb::State *m_state;
	ad2usb::Connection *m_connection;
	ad2usb::Parser *m_parser;
};

}

#endif
