#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errwrap.h"

void
unix_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int
Socket(int domain, int type, int protocol)
{
	int ret;
	if((ret = socket(domain, type, protocol)) < 0)
		unix_error("Socket error");
	return ret;
}

int
Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;
	if((ret = bind(sockfd, addr, addrlen)) < 0) {
		unix_error("Bind error");
		close(sockfd);
	}
	return ret;
}

ssize_t
Sendto(int sockfd, const void *buf, size_t len, int flags,
       const struct sockaddr *dest_addr, socklen_t addrlen)
{
	ssize_t ret;
	if((ret = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) < 0)
		unix_error("Sendto error");
	return ret;
}

ssize_t
Recvfrom(int sockfd, void *buf, size_t len, int flags,
         struct sockaddr *dest_addr, socklen_t *addrlen)
{
	ssize_t ret;
	if((ret = recvfrom(sockfd, buf, len, flags, dest_addr, addrlen)) < 0)
		unix_error("Recvfrom error");
	return ret;
}

int
Open(const char *pathname, int flags)
{
	int ret;
	if((ret = open(pathname, flags)) < 0)
		unix_error("Open error");
	return ret;
}

int
Close(int fd)
{
	int ret;
	if((ret = close(fd)) < 0)
		unix_error("Close error");
	return ret;
}

ssize_t
Read(int fd, void *buf, size_t count)
{
	ssize_t ret;
	if((ret = read(fd, buf, count)) < 0)
		unix_error("Read error");
	return ret;
}

ssize_t
Write(int fd, const void *buf, size_t count)
{
	ssize_t ret;
	if((ret = write(fd, buf, count)) < 0)
		unix_error("Write error");
	return ret;
}

int
Chdir(const char *path)
{
	int ret;
	if((ret = chdir(path)))
		unix_error("Chdir error");
	return ret;
}

struct servent
*Getservbyname(const char *name, const char *proto)
{
	struct servent *ret;
	if(!(ret = getservbyname(name, proto))) {
		fprintf(stderr, "Error: getservbyname() error\n)");
		exit(EXIT_FAILURE);
	}
	return ret;
}

struct protoent
*Getprotobyname(const char *name)
{
	struct protoent *ret;
	if(!(ret = getprotobyname(name))) {
		fprintf(stderr, "Error: getprotobyname() error\n)");
		exit(EXIT_FAILURE);
	}
	return ret;
}

int
Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	int ret;
	if((ret = setsockopt(sockfd, level, optname, optval, optlen)) < 0)
		unix_error("Setsockopt error");
	return ret;
}
