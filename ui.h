#include "parser.h"
#include "connection.h"
#include "state.h"

#include "log.h"

#include <string>
#include <list>

#ifndef _UI_H
#define _UI_H

namespace ad2usb
{

class Panel;

class UI
{
public:
	UI(ad2usb::Panel *panel);
	~UI();

	void listenForConnections();
	void stopListening();

private:
	pthread_t m_listeningThreadId;
	bool m_running;

	ad2usb::Panel *m_panel;

	std::list<int> m_sockets;
	pthread_mutex_t m_mutex;

	bool parseCommand(std::string str, int socketnum);
	void sendStatus(int socketnum);
	bool isKeypadInput(std::string str);

	void registerSocket(int socketnum);
	void unregisterSocket(int socketnum);

	struct threadData
	{
		UI *This;
		int data;
	};


	static void *listenThreadWrapper(void *data) {
		struct threadData *td = (struct threadData *)data;
		td->This->listenThread(td);
		return NULL;
	}
	static void *uiThreadWrapper(void *data) {
		struct threadData *td = (struct threadData *)data;
		td->This->uiThread(td);
		return NULL;
	}

	//threaded functions
	void *listenThread(struct threadData *td);
	void *uiThread(struct threadData *td);
	int tcp_listen(int socketnum);
	int tcp_recv_setup();
	int tcp_read(int socket, char *buf, int buflen);
	void tcp_write(int socket, const char *data, int datalen);
	bool isDataAvailable(int seconds, int useconds, int socketnum);



}; //class UI

} //namespace ad2usb

#endif
