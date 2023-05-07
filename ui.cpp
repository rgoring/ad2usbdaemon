#include "ui.h"
#include "log.h"
#include "panel.h"

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>

#include <strings.h>

using namespace logging;

ad2usb::UI::UI(ad2usb::Panel *panel)
{
    m_running = true;
    m_panel = panel;
    pthread_mutex_init(&m_mutex, NULL);
}

ad2usb::UI::~UI()
{
    m_running = false;
}

void ad2usb::UI::stopListening()
{
    m_running = false;

    //close all the registered sockets
    pthread_mutex_lock(&m_mutex);
    for (std::list<int>::iterator it = m_sockets.begin(); it != m_sockets.end(); it++) {
        shutdown(*it, SHUT_RDWR);
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Cleaning up socket " << *it << endl;
    }
    //clear the list
    m_sockets.erase(m_sockets.begin(), m_sockets.end());
    pthread_mutex_unlock(&m_mutex);

    //TODO: wait for all the threads to exit...
}

void ad2usb::UI::registerSocket(int socketnum)
{
    pthread_mutex_lock(&m_mutex);
    m_sockets.push_back(socketnum);
    pthread_mutex_unlock(&m_mutex);
}

void ad2usb::UI::unregisterSocket(int socketnum)
{
    //find the socket
    pthread_mutex_lock(&m_mutex);
    for (std::list<int>::iterator it = m_sockets.begin(); it != m_sockets.end(); it++) {
        if (*it == socketnum) {
            m_sockets.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

void ad2usb::UI::sendStatus(int socketnum)
{
    std::string statusStr;

    ad2usb::State *state = m_panel->getState();

    statusStr = Parser::stateToString(state->getAlarmState());
    statusStr.append(";");
    statusStr.append(state->getPanelMessage());
    statusStr.append("\n");

    tcp_write(socketnum, statusStr.c_str(), statusStr.length());
}

bool ad2usb::UI::isKeypadInput(std::string str)
{
    int i;

    for (i = 0; i < str.length(); i++)
    {
        if ((str[i] < '0' || str[i] > '9') && str[i] != '*' && str[i] != '#') {
            return false;
        }
    }
    return true;
}

bool ad2usb::UI::parseCommand(std::string str, int socketnum)
{
    bool isOk = true;
    ad2usb::Connection *con = m_panel->getConnection();

    if (isKeypadInput(str)) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Received keypad input" << endl;
        con->writeKeypadData(str);
    } else if (str.compare("status") == 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Received status command" << endl;
        sendStatus(socketnum);
    } else if (str.compare("stop") == 0) {
        log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Received stop command" << endl;
        m_panel->stopMonitoring();
        m_running = false;
    } else if (str.compare("") == 0) {
        //do nothing.. blank command sent.. timeout will handle it
    } else if (str.compare("close") == 0) {
        isOk = false; //close the connection
    } //ignore any other bad commands... timeout will eventually hit

    return isOk;
}

//creates the main listening thread and returns to main context
void ad2usb::UI::listenForConnections()
{
    struct threadData *td = new struct threadData();

    td->This = this;
    td->data = 0;

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI listen for connections" << endl;

    pthread_create(&m_listeningThreadId, NULL, listenThreadWrapper, td);
}

//main thread which distributes incoming connections
void *ad2usb::UI::listenThread(struct threadData *td)
{
    pthread_t threadid;
    int client_socket = 0;
    int server_socket = 0;
    struct threadData *client;

    delete td;

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI thread listening" << endl;

    server_socket = tcp_recv_setup();
    registerSocket(server_socket);

    while (this->m_running) {
        client_socket = tcp_listen(server_socket);
        if (client_socket < 0) {
            continue;
        } else {
            client = new struct threadData();
            client->This = this;
            client->data = client_socket;
            pthread_create(&threadid, NULL, uiThreadWrapper, (void *)client);
        }
    }

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI main thread finished listening" << endl;

    unregisterSocket(server_socket);
    shutdown(server_socket, SHUT_RDWR);

    pthread_exit(0);
}

//thread for incoming UI connections
void *ad2usb::UI::uiThread(struct threadData *td)
{
    char buf[100];
    int len = 0;
    int socketnum = td->data;
    int count = 0;
    bool keepParsing = true;

    delete td;

    registerSocket(socketnum);

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI thread new connection on socket " << socketnum << endl;

    while (keepParsing && m_running) {
        if (!isDataAvailable(1,0,socketnum)) {
            count++;
            if (count == 30) { //timeout to receive command
                log.getCategoryLogger(Log::LOG_INFO) << lprefix << "UI thread timeout" << endl;
                break;
            }
            continue;
        }
        if (!m_running) {
            continue;
        }

        count = 0; //reset the timeout

        if ((len = tcp_read(socketnum, buf, sizeof(buf))) < 0) {
            keepParsing = false;
            continue;
        }

        //strip off whitespace
        int i = 1;
        while(len - i >= 0) {
            if (buf[len-i] == '\n' || buf[len-i] == '\r' || buf[len - i] == ' ') {
                buf[len-i] = '\0';
                i++;
            } else {
                break;
            }
        }

        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Received data: " << buf << endl;

        keepParsing = parseCommand(std::string(buf), socketnum);
    }

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI client thread exiting." << endl;

    unregisterSocket(socketnum);
    shutdown(socketnum, SHUT_RDWR);

    pthread_exit(0);
}

bool ad2usb::UI::isDataAvailable(int seconds, int useconds, int socketnum)
{
    struct timeval timeout; // timeout param
    bool flag = false;

    fd_set fdvar;
    timeout.tv_sec = seconds;
    timeout.tv_usec = useconds;
    FD_ZERO(&fdvar);
    FD_SET(socketnum, &fdvar);
    select(socketnum+1, (fd_set *) &fdvar, (fd_set *) 0, (fd_set *) 0, &timeout);
    if(FD_ISSET(socketnum, &fdvar)) {
        flag = true;
    }
    return flag;
}

int ad2usb::UI::tcp_listen(int socketnum)
{
    int client_socket = 0;
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    // listen for a new client
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI listening on socket " << socketnum << endl;
    if (listen(socketnum, 5) < 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Listen call failed" << endl;
        return -1;
    }

    // accept them
    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "UI accepting client" << endl;
    if ((client_socket = accept(socketnum, (struct sockaddr*)&cli_addr, &cli_len)) < 0) {
        // NOTE: this is a blocking call, so shutdown() will break the thread that is blocked and "fail"
        //log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Accept call failed" << endl;
        return -2;
    }

    log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Client accepted" << endl;

    return client_socket;
}

int ad2usb::UI::tcp_recv_setup()
{
    /*
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "ERROR opening socket" << endl;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 22122;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "ERROR on binding" << endl;
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "ERROR on accept" << endl;
    }
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix <<  "ERROR reading from socket" << endl;
    };
    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);
    if (n < 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "ERROR writing to socket" << endl;
    }
    close(newsockfd);
    close(sockfd);
    */


    int server_socket = 0;
    struct sockaddr_in serv_addr; // socket address for local side
    socklen_t len = sizeof(serv_addr); //length of local address
    int option = 1;

    // create the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Socket call failed" << endl;
        exit(-1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //internet family
    serv_addr.sin_addr.s_addr = INADDR_ANY; //wild card machine address
    serv_addr.sin_port = htons(22122);

    //make the socket reusable
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, (char*)&option, sizeof(option)) < 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "setsockopt call failed" << endl;
        exit(-1);
    }

    //bind the name (address) to a port
    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Bind call failed" << endl;
        exit(-1);
    }

    //get the port name and print it out
    // if (getsockname(server_socket, (struct sockaddr*)&serv_addr, &len) < 0) {
    //     log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "Getsockname call failed" << endl;
    //     exit(-1);
    // }

    log.getCategoryLogger(Log::LOG_INFO) << lprefix << "Listening on port " << ntohs(serv_addr.sin_port) << endl;

    return server_socket;
}

// read the given socket into the buffer
int ad2usb::UI::tcp_read(int socket, char *buf, int buflen)
{
    int s;

    // read the socket
    if((s = recv(socket, buf, buflen, 0)) < 0) {
        log.getCategoryLogger(Log::LOG_ERROR) << lprefix << "recv call failed" << endl;
        return -1;
    } else if(s == 0) {
        log.getCategoryLogger(Log::LOG_DEBUG) << lprefix << "Peer closed connection." << endl;
        return -2;
    }

    return s;
}

// tcp_send actually sends the data on the socket. it takes the
//   socket as a parameter and also the packet to be sent.
void ad2usb::UI::tcp_write(int socket, const char *data, int datalen)
{
    send(socket, data, datalen, 0);
}
