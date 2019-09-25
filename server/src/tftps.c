#include <arpa/inet.h>
#include <grp.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "errwrap.h"
#include "packt.h"

#define TIMEOUT 30
#define RETRIES 10

void 
drop_privilege(void)
{
	gid_t newgid = getgid(), oldgid = getegid();
	uid_t newuid = getuid(), olduid = geteuid();
	if (!olduid) setgroups(1, &newgid);

	if (newgid != oldgid) {
		setegid(newgid);
		if (setgid(newgid) == -1) abort();
	}

	if (newuid != olduid) {
		seteuid(newuid);
		if (setuid(newuid) == -1) abort();
	}
}

void
handleRRQ(PACKT msg, struct sockaddr_in *client, socklen_t *socklen)
{
	int sock, fd;
	uint8_t data[MAX_DATA_PACKET_SIZE];
	uint16_t blknum;
	size_t datlen, i;

	struct protoent *protocol;
	struct timeval timelim;

	char *filename, *mode;
	PACKT datasend, response;

	protocol = Getprotobyname("udp");
	sock = Socket(AF_INET, SOCK_DGRAM, protocol->p_proto);

	timelim.tv_sec = TIMEOUT;

	(void)Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timelim, sizeof(timelim));

	filename = msg.rrq.filemode;
	mode = &msg.rrq.filemode[strlen(filename+2)]; /* ignored, if I get motivated I'll handle netascii */


	fd = Open(filename, O_RDONLY);
	blknum = 0;

	while((datlen = Read(fd, data, MAX_DATA_PACKET_SIZE))) {
		for(i = 0; i < RETRIES; i++) {
			datasend = make_data(blknum, data, datlen);
			send_packt(sock, &datasend, datlen+4, client, *socklen);

			recv_packt(sock, &response, client, socklen);
			if(ntohs(response.opcode) == OP_ERROR) {
				fprintf(stderr, "%s.%u: received error message: errno = %d, string: %s\n",
					inet_ntoa(client->sin_addr), ntohs(client->sin_port),
					ntohs(response.error.errcode), response.error.message);
			} else if(ntohs(response.ack.blk) == blknum){
				break;
			} else {
				fprintf(stderr, "%s.%u: received ack for wrong blknum %d\n",
					inet_ntoa(client->sin_addr), ntohs(client->sin_port),
					ntohs(response.ack.blk));
			}
		}

		if(i == RETRIES) {
			fprintf(stderr, "%s.%u: failed to send block %d.\n",
				inet_ntoa(client->sin_addr), ntohs(client->sin_port), blknum);
			exit(EXIT_FAILURE);
		}

		blknum++;
	}

	(void)Close(fd);
}

static void *
handleWRQ(void *args)
{
	PACKT *msg = args;

}

int
main(int argc, char **argv)
{
	struct servent *service;
	struct protoent *protocol;
	struct sockaddr_in sock_str;
	int sock;

	if(argc < 2 || argc > 2) {
		fprintf(stderr, "Usage:\n\t%s [directory]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	(void)Chdir(argv[1]);
	service = Getservbyname("tftp", "udp");
	protocol = Getprotobyname("udp");

	sock = Socket(AF_INET, SOCK_DGRAM, protocol->p_proto);

	sock_str.sin_family = AF_INET;
	sock_str.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_str.sin_port = service->s_port;

	(void)Bind(sock, (struct sockaddr *)&sock_str, sizeof(sock_str));

	printf("port: %d, protocol: %s\n", ntohs(service->s_port), protocol->p_name);

	drop_privilege();

	for(;;) {
		struct sockaddr_in client;
		socklen_t socklen = sizeof client;
		ssize_t packtlen;
		PACKT msg;
		uint16_t opcode;

		packtlen = recv_packt(sock, &msg, &client, &socklen);

		if(packtlen < 4) {
			fprintf(stderr, "%s.%u: message lenght less than minimum message size.\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			continue;
		}

		opcode = ntohs(msg.opcode);

		switch(opcode) {
		case OP_RRQ:
			fprintf(stderr, "%s.%u: received request type RRQ, filename = %s\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port), msg.rrq.filemode);
			if(!fork()) {
				handleRRQ(msg, &client, &socklen);
				exit(EXIT_SUCCESS);
			}
			break;
		case OP_WRQ:
			/* algo 2 */
			break;
		default:
			fprintf(stderr, "%s.%u: received opcode %hd.\n", inet_ntoa(client.sin_addr),
				ntohs(client.sin_port), opcode);
			continue;
		}
	}

	exit(EXIT_SUCCESS);
}