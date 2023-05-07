#include "panelinfo.h"

#ifndef _SERVER_H
#define _SERVER_H

namespace ad2usb
{

class Server
{
public:
	struct ControlFlags
	{
		bool *main_exit;
		PanelInfo::alarmStatus *status;
	};

	Server(struct ControlFlags *flags);
	~Server();

	void listenForConnections();
	void attachConnection();

private:
	struct ControlFlags *m_flags;

}; //class Server

} //namespace ad2usb

#endif
