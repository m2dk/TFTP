#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errwrap.h"
#include "packt.h"

PACKT
make_ack(uint16_t blkn)
{
	PACKT ackpkg;

	ackpkg.opcode = htons(OP_ACK);
	ackpkg.ack.blk = htons(blkn);

	return ackpkg;
}

PACKT
make_data(uint16_t blkn, uint8_t *dat, size_t datlen)
{
	if(datlen > MAX_DATA_PACKET_SIZE) {
		fprintf(stderr, "make_data: can't make data packet of that size.\n");
		exit(EXIT_FAILURE);
	}

	PACKT datapkg;

	datapkg.opcode = htons(OP_DATA);
	datapkg.data.blkn = htons(blkn);
	memcpy(datapkg.data.datablk, dat, datlen);

	return datapkg;
}

PACKT
make_err(uint16_t errcode, char *errmsg)
{
	if(strlen(errmsg) > MAX_STRING_SIZE) {
		fprintf(stderr, "make_err: can't make error packet of that size.\n");
		exit(EXIT_FAILURE);
	}

	PACKT errpkg;

	errpkg.opcode = htons(OP_ERROR);
	errpkg.error.errcode = htons(errcode);
	memcpy(errpkg.error.message, errmsg, strlen(errmsg));

	return errpkg;
}

ssize_t
recv_packt(int sock, void *msg, struct sockaddr_in *sin, socklen_t *slen)
{
	ssize_t packtlen = Recvfrom(sock, msg, sizeof(PACKT), 0, (struct sockaddr *)sin, slen);

	if(packtlen < 4) {
		fprintf(stderr, "%s.%u: message length less than minimum message size.\n",
			inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
		return -1;
	}

	return packtlen;
}

ssize_t
send_packt(int sock, void *msg, size_t msglen, struct sockaddr_in *sin, socklen_t slen)
{
	return Sendto(sock, msg, msglen, 0, (struct sockaddr *)sin, slen);
}