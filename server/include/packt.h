#include <stdint.h>

#define MAX_MODE_SIZE 8
#define MAX_DATA_PACKET_SIZE 512
#define MAX_STRING_SIZE 1023

/* OPCODES */
enum opcodes {
		OP_RRQ = 1,	/* 1 - Read Request  */
		OP_WRQ,		/* 2 - Write Request */
		OP_DATA,	/* 3 - Data Packet   */
		OP_ACK,		/* 4 - Acknowledge   */
		OP_ERROR	/* 5 - Error         */
};

/* ERRCODES */
enum errcodes {
		ER_UNDEF,	/* 0 - Undefined                        */
		ER_FNF,		/* 1 - File not found                   */
		ER_ACCVIOL,	/* 2 - Access violation                 */
		ER_NOALLOC,	/* 3 - Disk full or allocation exceeded */
		ER_ILLOP,	/* 4 - Illegal operation                */
		ER_UNKTID,	/* 5 - Unknown transfer ID              */
		ER_FAE,		/* 6 - File already exists              */
		ER_NOUSER	/* 7 - No such user                     */
};

/* Packet types */
typedef struct {
/*	char filename[MAX_STRING_SIZE+1];
	char mode[MAX_MODE_SIZE+1];*/
	char filemode[MAX_STRING_SIZE+MAX_MODE_SIZE+2];
} RRQ, WRQ;

typedef struct {
	uint16_t blkn;
	char datablk[MAX_DATA_PACKET_SIZE];
} DATA;

typedef struct {
	uint16_t blk;
} ACK;

typedef struct {
	uint16_t errcode;
	char message[MAX_STRING_SIZE+1];
} ERR;

typedef struct {
	uint16_t opcode;
	union {
		RRQ rrq;
		WRQ wrq;
		DATA data;
		ACK ack;
		ERR error;
	};
} PACKT;

PACKT make_ack(uint16_t);
PACKT make_data(uint16_t, uint8_t *, size_t);
PACKT make_err(uint16_t, char *);
ssize_t recv_packt(int, void *, struct sockaddr_in *, socklen_t *);
ssize_t send_packt(int, void *, size_t , struct sockaddr_in *, socklen_t);
