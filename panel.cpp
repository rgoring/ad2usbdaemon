#include "panel.h"
#include "log.h"

#include "connection.h"
#include "state.h"
#include "parser.h"
#include "ui.h"
#include "action.h"

#include <string>

using namespace logging;

ad2usb::Panel::Panel(const char *device)
{
	m_connection = new Connection(device);
	m_parser = new Parser();
	m_state = new State();
	m_ui = new UI(this);
}

ad2usb::Panel::~Panel()
{
	delete m_ui;
	delete m_connection;
	delete m_state;
	delete m_parser;

	m_ui = NULL;
	m_connection = NULL;
	m_state = NULL;
	m_parser = NULL;
}

ad2usb::State *ad2usb::Panel::getState()
{
	return m_state;
}

ad2usb::Connection *ad2usb::Panel::getConnection()
{
	return m_connection;
}

void ad2usb::Panel::startMonitoring()
{
	int timeoutCounter = 0;
	int failCounter = 0;
	std::string raw;
	Parser::alarmStatus status;

	m_monitoring = true;

    if (!m_connection->connect()) {
        return;
    }

    m_ui->listenForConnections();

    while (m_monitoring) {
        //read from the connection
        if (!m_connection->isDataAvailable()) {
            timeoutCounter++;
            if (timeoutCounter >= 100) { //100 seconds with no response from alarm
                m_connection->reconnect();
                timeoutCounter = 0;
            }
            continue;
        }
        if (!m_monitoring) {
            continue;
        }
        timeoutCounter = 0;
        if (!m_connection->getrawoutput(raw)) {
            if (failCounter == 100) {
                m_connection->reconnect();
                failCounter = 0;
            }
            continue;
        }
        failCounter = 0;

        LOG(Log::LOG_DEBUG) << lprefix << raw << endl;
        
        //parse the message
        if (!m_parser->parseRawData(raw, &status)) {
        	continue;
        }
        
        //send to the state maintainer
        m_state->setState(status);
    }

    m_ui->stopListening();
}

void ad2usb::Panel::stopMonitoring()
{
	m_monitoring = false;
}

void ad2usb::Panel::addAction(Panel::alarmEvent event, ad2usb::Action *a)
{
	m_state->addAction(event, a);
}