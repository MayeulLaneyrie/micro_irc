/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/06/28 18:06:31 by mlaneyri         ###   ########.fr       */
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

#define MAXEV 10
#define BUFFER_SIZE 1024

int die(const char * fmt, ...) {
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
	return (0);
}

int setsock_nonblock(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	return (0);
}

std::string itos(int n) {
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

std::string username(int fd) {
	std::string ret("\e[1;");
	ret.append(itos(91 + fd % 6)).append("m[").append(itos(fd)).append("]\e[0m ");
	return (ret);
}

int broadcast(std::map<int, std::string> const & clients, std::string const & msg, int except) {
	std::map<int, std::string>::const_iterator it = clients.begin();
	while (it != clients.end()) {
		if (it->first == except) {
			++it;
			continue ;
		}
		send(it->first, msg.c_str(), msg.size(), 0);
		++it;
	}
	return (0);
}

int command(std::map<int, std::string> & clients, int sender) {
	size_t comstart = clients[sender].find(' ') + 1;
	std::string rawcmd = clients[sender].substr(comstart, clients[sender].size() - comstart);

	if (rawcmd == "/part\n") {
		close(sender);
		clients.erase(sender);
		std::cout << "\e[1m[PART]\e[0m " << username(sender) << std::endl;
		broadcast(clients,
				std::string("\e[1m[SERVER]\e[0m ")
					.append(username(sender))
					.append("left.\n"), -1);
	}
	else if (rawcmd == "/stop\n") {
		std::cout << "\e[1m[STOP]\e[0m " << clients[sender];
		broadcast(clients,
				std::string("\e[1m[SERVER]\e[0m ")
					.append(username(sender))
					.append("stopped the server.\n"), -1);
		return (1);
	}
	else {
		std::cout << "\e[1m[MSG]\e[0m " << clients[sender];
		broadcast(clients, clients[sender], sender);
		clients[sender] = username(sender);
	}
	return (0);
}

int main(void) {
	
	struct sockaddr_in sa;
	int sockfd, addrlen = sizeof(sa);
	char buffer[BUFFER_SIZE];

	sa.sin_family = AF_INET;
	sa.sin_port = htons(6667);
	sa.sin_addr.s_addr = INADDR_ANY;

// MAIN SOCKET SETUP -----------------------------------------------------------

	if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));
	if (listen(sockfd, 4) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));

	setsock_nonblock(sockfd);

// EPOLL SETUP -----------------------------------------------------------------

	struct epoll_event ev, evts[MAXEV];
	int epollfd, nfds, curfd, stop = 0;

	std::map<int, std::string> clients;

	if ((epollfd = epoll_create(10)) < 0) // Size parameter is nowadays unused
		die("sds", __FILE__, __LINE__, strerror(errno));

	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
		die("sds", __FILE__, __LINE__, strerror(errno));

// -----------------------------------------------------------------------------

	buffer[0] = '\0';

// MAIN LOOP START -------------------------------------------------------------

	while (!stop) {

		if ((nfds = epoll_wait(epollfd, evts, MAXEV, -1)) < 0)
			die("sds", __FILE__, __LINE__, strerror(errno));

		// Traitement des évènements
		for (int i = 0; i < nfds; ++i) {

			curfd = evts[i].data.fd;

			if (curfd == sockfd) { // Nouvelle connection ----------------------

				if ((curfd = 
							accept(sockfd, (struct sockaddr *) &sa,
								(socklen_t *) &addrlen)) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				setsock_nonblock(curfd);

				ev.events = EPOLLIN;
				ev.data.fd = curfd;

				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, curfd, &ev) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				clients[curfd] = username(curfd);

				std::cout << "\e[1m[JOIN]\e[0m " << clients[curfd] << std::endl;
				
				broadcast(clients,
					std::string("\e[1m[SERVER]\e[0m ")
						.append(username(curfd)).append("joined.\n"), curfd);
			}
			else { // Cas général, msg -----------------------------------------

				int len = recv(curfd, buffer, BUFFER_SIZE - 1, 0);
				if (!len) {
					close(curfd);
					clients.erase(curfd);
					std::cout
						<< "\e[1m[CLOSE]\e[0m " << username(curfd) << std::endl;
				}
				else {
					buffer[len] = '\0';
					clients[curfd].append(buffer);

					if (buffer[len - 1] == '\n')
						stop = command(clients, curfd);
					else
						std::cout << "\e[1m...\e[0m " << buffer << std::endl;
				}
			}
		}
	}

	close(sockfd);
	return (0);
}
