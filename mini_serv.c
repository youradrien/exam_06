#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

int max_fd = 0, count = 0;
int ids[65536];
char *msgs[65536];

fd_set rfds, wfds, afds;

char buf_read[1001];
char buf_write[42];

static void	fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

static void	notify_other(int author, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, str, strlen(str), 0);
	}
}

// int extract_message(char **buf, char **msg)
// {
// 	char	*newbuf;
// 	int	i;

// 	*msg = 0;
// 	if (*buf == 0)
// 		return (0);
// 	i = 0;
// 	while ((*buf)[i])
// 	{
// 		if ((*buf)[i] == '\n')
// 		{
// 			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
// 			if (newbuf == 0)
// 				return (-1);
// 			strcpy(newbuf, *buf + i + 1);
// 			*msg = *buf;
// 			(*msg)[i + 1] = 0;
// 			*buf = newbuf;
// 			return (1);
// 		}
// 		i++;
// 	}
// 	return (0);
// }

// char *str_join(char *buf, char *add)
// {
// 	char	*newbuf;
// 	int		len;

// 	if (buf == 0)
// 		len = 0;
// 	else
// 		len = strlen(buf);
// 	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
// 	if (newbuf == 0)
// 		return (0);
// 	newbuf[0] = 0;
// 	if (buf != 0)
// 		strcat(newbuf, buf);
// 	free(buf);
// 	strcat(newbuf, add);
// 	return (newbuf);
// }

static void	remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);
	free(msgs[fd]);
	FD_CLR(fd, &afds);
	close(fd);
}

static void	send_message(int fd)
{
	char *bfr;

	while (extract_message(&(msgs[fd]), &bfr))
	{
		sprintf(buf_write, "client %d: ", ids[fd]);
		notify_other(fd, buf_write);
		notify_other(fd, bfr);
		free(bfr);
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	// socket
	FD_ZERO(&afds);
	int sockfd;
	struct sockaddr_in servaddr; 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd  < 0)
		fatal_error();
	bzero(&servaddr, sizeof(servaddr)); 
	FD_SET(sockfd, &afds);
	max_fd = sockfd;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0 
		|| listen(sockfd, SOMAXCONN) != 0)
		fatal_error();

	while (1)
	{
		rfds = wfds = afds;
		if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))
				continue;
			if (fd == sockfd)
			{
				struct sockaddr_in cliaddr;
				socklen_t len_addr = sizeof(cliaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&cliaddr, &len_addr);
				if (client_fd >= 0) {
					max_fd = client_fd > max_fd ? client_fd : max_fd;
					ids[client_fd] = count++;
					msgs[client_fd] = NULL;
					FD_SET(client_fd, &afds);
					sprintf(buf_write, "server: client %d just arrived\n", ids[client_fd]);
					notify_other(client_fd, buf_write);
				}
			}
			else
			{
				int read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)
				{
					remove_client(fd);
					continue;
				}
				buf_read[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], buf_read);
				send_message(fd);
			}
		}
	}
	return (0);
}
