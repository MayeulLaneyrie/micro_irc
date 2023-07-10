/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/07/10 15:17:07 by mlaneyri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#define MAXEV 16
#define BUFFER_SIZE 1024

/* die() =======================================================================
 *
 *	Exits the program, after printing a formatted error message.
 *	I use it to protect most syscalls, almost always with the following call:
 *		die("sds", __FILE__, __LINE__, strerror(errno));
 */

void die(const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	int d;
	char c;
	char *s;
	double f;

	while (*fmt) {
		switch (*fmt) {
			case 'd':
				d = va_arg(args, int);
				std::cerr << d;
				break ;
			case 'c':
				d = va_arg(args, int);
				c = d;
				std::cerr << c;
				break ;
			case 's':
				s = va_arg(args, char *);
				std::cerr << s;
				break ;
			case 'f':
				f = va_arg(args, double);
				std::cerr << f;
		}
		++fmt;
		if (*fmt)
			std::cerr << ": ";
	}
	std::cerr << std::endl;
	exit(EXIT_FAILURE);
}

/* setsock_nonblock() ==========================================================
 *
 *	Quite self-explanatory. Sets a fd in non-blocking mode.
 */

void setsock_nonblock(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
}

/* itos() ======================================================================
 *
 * int --> std::string
 *
 * Return value: the converted string.
 */

std::string itos(int n) {
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

/* username() ==================================================================
 *
 * Creates a formatted, colored username according to the fd send.
 * The resulting string will precede all messages sent by that fd.
 *
 * Return value: the string containing the formatted username.
*/

std::string username(int fd) {
	std::string ret("\e[1;");

	ret.append(itos(91 + fd % 6)).append("m[").append(itos(fd)).append("]\e[0m ");
	return (ret);
}

/* broadcast() =================================================================
 *
 * Send a message to all clients contained in 'clients', except 'except'
 * (which is supposed to be the sender).
 */

void broadcast(std::map<int, std::string> const & clients, std::string const & msg, int except) {
	std::map<int, std::string>::const_iterator it;

	for (it = clients.begin; it != clients.end(); ++it)
		if (it->first != except)
			send(it->first, msg.c_str(), msg.size(), 0);
}

/* command() ===================================================================
 *
 * Execute a command send by a client. 'sender' is the fd of the client we want
 * to execute the command from. The command will be read and parsed from the
 * client's own input buffer, that is stored in 'clients'.
 *
 * The available commands are :
 * 	- /part: disconnects the client from the server.
 * 	- /stop: stops the server.
 * 	- Anything else: broadcast a message to all clients.
 *
 * 	Return value: boolean, whether the server should stop.
 */

int command(std::map<int, std::string> & clients, int sender) {
	if (clients[sender] == "/part\n") {
		send(sender, "\e[1m[SERVER]\e[0m You left.\n", 27, 0);
		close(sender);
		clients.erase(sender);
		std::cout << "\e[1m[PART]\e[0m " << username(sender) << std::endl;
		broadcast(clients,
				std::string("\e[1m[SERVER]\e[0m ")
					.append(username(sender))
					.append("left.\n"), -1);
	}
	else if (clients[sender] == "/stop\n") {
		std::cout << "\e[1m[STOP]\e[0m " << username(sender) << std::endl;
		broadcast(clients,
				std::string("\e[1m[SERVER]\e[0m ")
					.append(username(sender))
					.append("stopped the server.\n"), -1);
		return (1);
	}
	else {
		std::cout << "\e[1m[MSG]\e[0m " << username(sender) << clients[sender];
		broadcast(clients, clients[sender].insert(0, username(sender)), sender);
		clients[sender] = "";
	}
	return (0);
}

/*
 * main() ======================================================================
 *
 * Let's start the serious business.
 */

int main(void) {
	
	struct sockaddr_in sa;
	int sockfd, addrlen = sizeof(sa);

	sa.sin_family = AF_INET;
	sa.sin_port = htons(6667); // 6667 is the default port for irc.
	sa.sin_addr.s_addr = INADDR_ANY;

	/*
	 * Setting up 'sockfd', the listening socket, that will allow us to accept
	 * new connections. Think of it as a big cable, which itself is holding up
	 * smaller cables. We first create it, then bind it to the adress and port
	 * contained in 'sa'. We then make it listen on that port. Any new connection
	 * request on port 6667 will be received on sockfd.
	 */

	if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (listen(sockfd, 64) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));

	setsock_nonblock(sockfd);

	/*
	 * Setting up epoll. Since our socket are all made non-blocking using
	 * setsock_nonblock() (see implementation higher), we could just do a loop
	 * of recv() and accept(), and do something only when we read something. But
	 * that would be very inefficient. In fact, we would like to run the loop
	 * and call recv() or accept() only when we know for sure there is something
	 * to read on one of our connections, or a new connection to accept.
	 *
	 * So we will use epoll_wait, to block the program while there is nothing to
	 * do. Once one or multiple events have been detected, they will be stored
	 * in an array for us to manage.
 	 */

	int epollfd;

	if ((epollfd = epoll_create(10)) < 0) // Size parameter is ignored, should be >0
		die("sds", __FILE__, __LINE__, strerror(errno));

	/*
	 * Here, we register 'sockfd' to be checked by epoll.
	 */
	struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));

	/*
	 * Starting the main server loop. Each client will be identified only by his
	 * fd. Each client will have his own input buffer, to manage partial
	 * commands/msgs (e.g. when the message size is higher than BUFFER_SIZE).
	 * All this is stored in the 'client' map, indexed by their fds.
	 */

	std::map<int, std::string> clients;

	char buffer[BUFFER_SIZE];
	buffer[0] = '\0';
	
	int stop = 0;

	while (!stop) {

		int nfds;
		struct epoll_event evts[MAXEV];

		if ((nfds = epoll_wait(epollfd, evts, MAXEV, -1)) < 0)
			die("sds", __FILE__, __LINE__, strerror(errno));

		/*
		 * Let's manage all our events.
		 */
		for (int i = 0; i < nfds; ++i) {

			int currentfd = evts[i].data.fd;

			/*
			 * If the fd to manage is the listening socket, it means there is a
			 * new connection to accept and setup.
			 *
			 * To do list:
			 * 	- Set the fd as non blocking.
			 * 	- Register it to epoll.
			 * 	- Other setup, notifying the other clients.
			 */
			if (currentfd == sockfd) {

				if ((currentfd = 
							accept(sockfd, (struct sockaddr *) &sa,
								(socklen_t *) &addrlen)) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				setsock_nonblock(currentfd);

				ev.events = EPOLLIN;
				ev.data.fd = currentfd;

				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, currentfd, &ev) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				clients[currentfd] = "";

				std::cout << "\e[1m[JOIN]\e[0m " << username(currentfd) << std::endl;
				
				broadcast(clients,
					std::string("\e[1m[SERVER]\e[0m ")
						.append(username(currentfd)).append("joined.\n"), currentfd);
			}
			/*
			 * General case, we are receiving a command or simple message from
			 * one of our existing connections.
			 */
			else {

				int len = recv(currentfd, buffer, BUFFER_SIZE - 1, 0);
				if (!len) {
					close(currentfd);
					clients.erase(currentfd);
					std::cout
						<< "\e[1m[CLOSE]\e[0m " << username(currentfd) << std::endl;
				}
				else {
					buffer[len] = '\0';
					clients[currentfd].append(buffer);

					if (buffer[len - 1] == '\n') // Complete cmd/msg, manage it.
						stop = command(clients, currentfd);
					else // Incomplete cmd/msg, we do nothing until it's complete.
						std::cout << "\e[1m...\e[0m " << buffer << std::endl;
				}
			}
		}
	}

	std::map<int, std::string>::const_iterator it;
	for (it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	close(epollfd);
	close(sockfd);

	return (0);
}
