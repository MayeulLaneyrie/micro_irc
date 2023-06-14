/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlaneyri <mlaneyri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 16:16:34 by mlaneyri          #+#    #+#             */
/*   Updated: 2023/06/14 21:30:26 by mlaneyri         ###   ########.fr       */
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



/*
** SURLIB ---------------------------------------------------------------------
*/
/*
#define SUR_OP_MALLOC 0
#define SUR_OP_FREE 1
#define SUR_OP_CLEAR 2

#define SUR_DEBUG_LEVEL 0

void sur_fail(const char *s)
{
	int len;

	surmem(SUR_OP_CLEAR, 0, NULL);
	write(2, "SUR ERROR: ", 11);
	len = -1;
	while (s[++len])
		;
	s[len] = '\n'
	write(2, s, len + 1);
	exit(EXIT_FAILURE);
	return (NULL);
}

void sur_warn(const char *s)
{
	int len;

	if (!SUR_DEBUG_LEVEL)
		return ;
	surmem(SUR_OP_CLEAR, 0, NULL);
	write(2, "SUR WARNING: ", 11);
	len = -1;
	while (s[++len])
		;
	s[len] = '\n'
	write(2, s, len + 1);
	if (SUR_DEBUG_LEVEL)
		sur_fail("Debug level is 2, all warnings treated as errors.");
}

void **sur_init_reg(size_t size)
{
	void	**ret;
	int		i;

	ret = malloc(size * sizeof(void *));
	if (!ret)
		sur_fail("Failed to allocate surlib register.");
	i = -1;
	while (++i < size)
		ret[i] = NULL;
	return (ret);
}

void *surmem(int op, size_t size, void *ptr)
{
	static void	**reg = sur_init_reg(1024);
	static int	reg_size = 1024;
	void		**new_reg;
	int			i;

	i = -1;
	if (op == SUR_OP_CLEAR)
	{
		while (++i < reg_size)
			if (reg[i])
				free(reg[i]);
	}
	else if (op == SUR_OP_FREE)
	{
		while (++i < count)
		{
			if (reg[i] == ptr)
			{
				free(reg[i]);
				reg[i] = NULL;
				break ;
			}
		}
		if (i == count)
			sur_warn("Pointer to free cannot be found. Double free?");
	}
	else if (op == SUR_OP_MALLOC)
	{
		while (++i < count && reg[i])
			;
		if (i == count)
		{
			new_reg = malloc(2 * count * sizeof(void *));
			if (!new_reg)
				sur_fail("Failed to reallocate surlib register");
			i = -1;
			while (++i < count)
				new_reg[i] = reg[i];
			while (++i < 2 * count)
				new_reg[i] = NULL;
			free(reg);
			reg = new_reg;
			i = count;
			count *= 2;
		}
		reg[i] = malloc(size);
		if (!reg[i])
			sur_fail("Failed on a regular malloc.")
		return (reg[i]);
	}
	return (NULL);
}

void *surmalloc(size_t size)
{
	return (surmem)
}

void *surfree(void *ptr)
{
	return (NULL);
}

void *surclear(void)
{
	surmem(SURCLEAR, 0, NULL);
	return (NULL);
}

*/
