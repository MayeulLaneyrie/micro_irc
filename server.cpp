/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/06/27 12:10:01 by mlaneyri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <vector>

#include <string.h>

#include <stdio.h>
#include <stdarg.h>

#include <sys/epoll.h>

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
	}
	std::cerr << '\n' << std::flush;
	exit(EXIT_FAILURE);
	return (0);
}

int main(void) {
	
	int sockfd;	
	struct sockaddr_in sa;
	int addrlen = sizeof(sa);
	char buffer[1024];

	sa.sin_family = AF_INET;
	sa.sin_port = htons(6667);
	sa.sin_addr.s_addr = INADDR_ANY;

	if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
		die("ss", "Critical failure when creating socket: ", strerror(errno));

	if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		die("ss", "Critical failure at bind: ", strerror(errno));

	if (listen(sockfd, 4) < 0)
		die("ss", "Critical failure at listen: ", strerror(errno));

//	std::vector<int> sockets;
//	int epoll_fd = epoll_create(1);

	int new_socket;

	buffer[0] = '\0';

	while (std::string(buffer) != "STOP\n") {

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
	}
	close(sockfd);
	return (0);
}
