#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include "sha1.h"
#define SERVER_PORT 47500
#define MAX_LINE 256
#define MAX_FILE_SIZE 12288

#define FLAG_HELLO		((uint8_t)(0x01 << 7))
#define FLAG_INSTRUCTION	((uint8_t)(0x01 << 6))
#define FLAG_RESPONSE		((uint8_t)(0x01 << 5))
#define FLAG_TERMINATE		((uint8_t)(0x01 << 4))

#define OP_ECHO		       	((uint8_t)(0x00))
#define OP_INCREMENT		((uint8_t)(0x01))
#define OP_DECREMENT		((uint8_t)(0x02))
#define OP_PUSH			((uint8_t)(0x03))
#define OP_DIGEST		((uint8_t)(0x04))

struct hw_packet {
  uint8_t  flag;
  uint8_t  operation;
  uint16_t data_len;
  uint32_t seq_num;
  uint8_t  data[1024];
};

void send_packet(int s, uint8_t flag, uint8_t op, uint16_t len, uint32_t seq, uint8_t *data);

int main(int argc, char * argv[]) {
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host = "localhost";
    int s, file_index = 0;
    char file[MAX_FILE_SIZE];

    /* translate host name into peer's IP address */
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }

    uint32_t student_id = 2015410012;
    uint8_t tmp[4];
    memcpy(tmp, &student_id, sizeof(uint32_t));
    send_packet(s, FLAG_HELLO, OP_ECHO, 4, 0, tmp);

    while(1) {
        struct hw_packet rcvd_packet;
        recv(s, (char*) &rcvd_packet, sizeof(struct hw_packet), 0);

        printf("rcvd flag : %02X\n", rcvd_packet.flag);
        printf("rcvd op   : %02X\n", rcvd_packet.operation);
        printf("rcvd len  : %04X\n", rcvd_packet.data_len);
        printf("rcvd seq  : %08X\n", rcvd_packet.seq_num);
        printf("rcvd data : ");
        int i;
        for(i = 0; i < rcvd_packet.data_len; i++)
            printf("%02X", rcvd_packet.data[i]);
        printf("\n\n");

        if(rcvd_packet.flag == FLAG_INSTRUCTION) {
            uint8_t op = rcvd_packet.operation;
            uint16_t len = rcvd_packet.data_len;
            uint32_t seq = rcvd_packet.seq_num;

            if(op == OP_INCREMENT || op == OP_DECREMENT) {
                uint32_t tmp;
                memcpy(&tmp, rcvd_packet.data, sizeof(uint32_t));
                tmp += (op == OP_INCREMENT) ? 1 : -1;
                memcpy(rcvd_packet.data, &tmp, sizeof(uint32_t));
            
                send_packet(s, FLAG_RESPONSE, OP_ECHO, len, seq, rcvd_packet.data);
            } else if(op == OP_PUSH) {
                printf("    len : %d\n", len);
                memcpy(&(file[file_index]), rcvd_packet.data, len);
                printf("    before file_index : %d\n", file_index);
                file_index += len;
                printf("    after file_index : %d\n\n", file_index);

                send_packet(s, FLAG_RESPONSE, OP_PUSH, 0, 0, NULL);
            } else if(op == OP_DIGEST) {
                char *hash_out = malloc(20);
                SHA1(hash_out, file, file_index);
                send_packet(s, FLAG_RESPONSE, OP_DIGEST, 20, 0, hash_out);
            }
        }

        if(rcvd_packet.flag == FLAG_TERMINATE)
            break;
    }

    return 0;
}

void send_packet(int s, uint8_t flag, uint8_t op, uint16_t len, uint32_t seq, uint8_t *data) {
    struct hw_packet send_packet;
    send_packet.flag = flag;
    send_packet.operation = op;
    send_packet.data_len = len;
    send_packet.seq_num = seq;
    if(data != NULL)
        memcpy(send_packet.data, data, len);

    printf("send flag : %02X\n", send_packet.flag);
    printf("send op   : %02X\n", send_packet.operation);
    printf("send len  : %04X\n", send_packet.data_len);
    printf("send seq  : %08X\n", send_packet.seq_num);
    printf("send data : ");
    int i;
    for(i = 0; i < send_packet.data_len; i++)
        printf("%02X", send_packet.data[i]);
    printf("\n\n");

    send(s, (char*) &send_packet, sizeof(struct hw_packet), 0);
}
