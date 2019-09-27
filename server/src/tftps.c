#include <arpa/inet.h>
#include <grp.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
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
	setgid(strtoll(getenv("SUDO_GID"), NULL, 10));
	setuid(strtoll(getenv("SUDO_UID"), NULL, 10));
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
	/* this is needed in some Linux kernels, for some reason */
	timelim.tv_usec = 0; 

	(void)Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timelim, sizeof(timelim));

	filename = msg.rrq.filemode;
	mode = &msg.rrq.filemode[strlen(filename)+1]; /* ignored, if I get motivated I'll handle netascii */

	fd = Open(filename, O_RDONLY);
	blknum = 1;
	datlen = MAX_DATA_PACKET_SIZE;

	while(datlen == MAX_DATA_PACKET_SIZE) {
		datlen = Read(fd, data, MAX_DATA_PACKET_SIZE); /* ugly workaround to send 0-length packets */
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
				fprintf(stderr, "%s.%u: received ack for wrong blknum %d, expecting %d\n",
					inet_ntoa(client->sin_addr), ntohs(client->sin_port),
					ntohs(response.ack.blk), blknum);
				continue;
			}
		}

		if(i == RETRIES) {
			fprintf(stderr, "%s.%u: failed to send block %d\n",
				inet_ntoa(client->sin_addr), ntohs(client->sin_port), blknum);
			exit(EXIT_FAILURE);
		}

		blknum++;
	}

	(void)Close(fd);
}

void
handleWRQ(PACKT msg, struct sockaddr_in *client, socklen_t *socklen)
{
	int sock, fd;
	uint8_t data[MAX_DATA_PACKET_SIZE];
	uint16_t blknum;
	ssize_t datlen, i;

	struct protoent *protocol;
	struct timeval timelim;

	char *filename, *mode;
	PACKT ack, response;

	protocol = Getprotobyname("udp");
	sock = Socket(AF_INET, SOCK_DGRAM, protocol->p_proto);

	timelim.tv_sec = TIMEOUT;
	/* this is needed in some Linux kernels, for some reason */
	timelim.tv_usec = 0; 

	(void)Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timelim, sizeof(timelim));

	filename = msg.wrq.filemode;
	mode = &msg.wrq.filemode[strlen(filename)+1]; /* ignored, if I get motivated I'll handle netascii */

	fd = Open(filename, O_WRONLY | O_CREAT);
	fchmod(fd, 0644);
	blknum = 0;

	do {
		ack = make_ack(blknum);
		
		for(i = 0; i < RETRIES; i++) {
			(void)send_packt(sock, &ack, 4, client, *socklen);

			datlen = recv_packt(sock, &response, client, socklen);
			if(ntohs(response.opcode) != OP_DATA) {
				fprintf(stderr, "%s.%u: received packet with opcode %d\n",
					inet_ntoa(client->sin_addr), ntohs(client->sin_port),
					ntohs(response.opcode));
				exit(EXIT_FAILURE);
			} else {
				break;
			}
		}

		if(i == RETRIES) {
			fprintf(stderr, "%s.%u: failed to receive block %d\n",
				inet_ntoa(client->sin_addr), ntohs(client->sin_port),
				blknum+1);
			exit(EXIT_FAILURE);
		}

		blknum = ntohs(response.data.blkn);
		Write(fd, response.data.datablk, datlen-4);
	} while(datlen-4 == MAX_DATA_PACKET_SIZE);

	ack = make_ack(blknum); /* ugly workaround to send last ack */
	(void)send_packt(sock, &ack, 4, client, *socklen);

	(void)Close(fd);
}

int
main(int argc, char **argv)
{
	struct servent *service;
	struct protoent *protocol;
	struct sockaddr_in sock_str;
	int sock;

	if(argc != 2) {
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

	printf("Server listening on port: %d.\n", ntohs(service->s_port));

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
			fprintf(stderr, "%s.%u: received request type WRQ, filename = %s\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port), msg.wrq.filemode);
			if(!fork()) {
				handleWRQ(msg, &client, &socklen);
				exit(EXIT_SUCCESS);
			}
			break;
		default:
			fprintf(stderr, "%s.%u: received opcode %hd.\n", inet_ntoa(client.sin_addr),
				ntohs(client.sin_port), opcode);
			continue;
		}
		signal(SIGCHLD, SIG_IGN); /* prevent zombie process */
	}

	exit(EXIT_SUCCESS);
}
