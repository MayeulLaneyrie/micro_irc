/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/05/22 21:19:32 by mlaneyri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <iostream>
#include <string>

int main(void) {
	
	struct sockaddr_in sa;
	int addrlen = sizeof(sa);
	char buffer[1024];

	sa.sin_family = AF_INET;
	sa.sin_port = htons(6667);
	sa.sin_addr.s_addr = INADDR_ANY;

	int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	bind(sockfd, (struct sockaddr *) &sa, sizeof(sa));

	listen(sockfd, 4);

	int i = 0;
	int new_socket;
	buffer[0] = '\0';
	while (std::string(buffer) != "STOP\n") {

		new_socket = -1;
		while (new_socket < 0)
			new_socket =
				accept(sockfd, (struct sockaddr *) &sa, (socklen_t *) &addrlen);

		if (!i)
			std::cout << "* * *" << std::endl;
		buffer[0] = '\0';
		while (std::string(buffer) != "NEXT\n" && std::string(buffer) != "STOP\n") 
		{

			int n = read(new_socket, buffer, 1023);
			buffer[n] = '\0';

			std::cout << i << ": \t" << buffer;

			send(new_socket, "*\n", 2, 0);

		}
		send(new_socket, "* * *\n", 6, 0);
		std::cout << "* * *" << std::endl;

		close(new_socket);
	
		++i;
	}

	close(sockfd);

	return (0);
}
