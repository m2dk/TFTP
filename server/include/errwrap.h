#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stddef.h>

void unix_error(char *);
int Socket(int, int, int);
int Bind(int, const struct sockaddr *, socklen_t);
ssize_t Sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
ssize_t Recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
int Open(const char *, int);
int Close(int);
ssize_t Read(int, void *, size_t);
ssize_t Write(int, const void *, size_t);
int Chdir(const char *);
struct servent *Getservbyname(const char *, const char *);
struct protoent *Getprotobyname(const char *);
int Setsockopt(int, int, int, const void *, socklen_t);
