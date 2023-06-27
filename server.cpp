/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/06/27 21:27:23 by mlaneyri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <string.h>

#include <stdio.h>
#include <stdarg.h>

#include <sys/epoll.h>

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

	struct epoll_event
		ev,
		evts[MAXEV];
	int
		epollfd,
		nfds,
		new_socket;

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

	while (std::string(buffer) != "STOP\n") {

// NEW SERVER LOOP -------------------------------------------------------------

		if ((nfds = epoll_wait(epollfd, evts, MAXEV, -1)) < 0)
			die("sds", __FILE__, __LINE__, strerror(errno));

		// Traitement des évènements
		for (int i = 0; i < nfds; ++i) {

			int curfd = evts[i].data.fd;

			if (curfd == sockfd) { // Nouvelle connection

				if ((new_socket = 
							accept(sockfd, (struct sockaddr *) &sa,
								(socklen_t *) &addrlen)) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				setsock_nonblock(new_socket);

				ev.events = EPOLLIN;
				ev.data.fd = new_socket;

				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_socket, &ev) < 0)
					die("sds", __FILE__, __LINE__, strerror(errno));

				clients[new_socket] = username(new_socket);

				std::cout
					<< "\e[1m[NEW CLIENT]\e[0m #" << new_socket << std::endl;
			}
			else { // Cas général, msg

				int len = recv(curfd, buffer, BUFFER_SIZE - 1, 0);
				if (!len) {
					close(curfd);
					clients.erase(curfd);
					std::cout
						<< "\e[1m[CLIENT PARTED]\e[0m " << curfd << std::endl;
				}
				else {
					buffer[len] = '\0';
					clients[curfd].append(buffer);

					if (buffer[len - 1] == '\n') {
						std::cout << "\e[1m[MSG]\e[0m " << clients[curfd];
						std::map<int, std::string>::iterator it = clients.begin();
						while (it != clients.end()) {
							if (it->first == curfd) {
								++it;
								continue ;
							}
							send(it->first, clients[curfd].c_str(), clients[curfd].size(), MSG_NOSIGNAL);
							++it;
						}
						clients[curfd] = username(curfd);
					}
					else
						std::cout << "\e[1;3m[PARTIAL MSG]\e[0m " << clients[curfd] << std::endl;

				}
			}
		}


// OLD SERVER LOOP -------------------------------------------------------------
/*
		new_socket = -1;
		while (new_socket < 0)
			new_socket =
				accept(sockfd, (struct sockaddr *) &sa, (socklen_t *) &addrlen);

	
		buffer[0] = '\0';
		
		while (std::string(buffer) != "NEXT\n"
				&& std::string(buffer) != "STOP\n") {

			send(new_socket, "\n* ", 3, MSG_NOSIGNAL);
			int n = recv(new_socket, buffer, 1023, 0);
			buffer[n] = '\0';

			if (!n)
				break ;
			std::cout << buffer << std::flush;
		}
		send(new_socket, "* * *\n", 6, 0);
		close(new_socket);
*/
	}
	close(sockfd);
	return (0);
}
