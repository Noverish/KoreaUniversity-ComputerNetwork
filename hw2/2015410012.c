#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#define SERVER_PORT 47500
#define MAX_LINE 256
#define BUF_SIZE 1024

#define FLAG_HELLO          ((uint8_t)(0x01 << 7))
#define FLAG_INSTRUCTION    ((uint8_t)(0x01 << 6))
#define FLAG_RESPONSE       ((uint8_t)(0x01 << 5))
#define FLAG_TERMINATE      ((uint8_t)(0x01 << 4))

#define OP_ECHO             ((uint8_t)(0x00))
#define OP_INCREMENT        ((uint8_t)(0x01))
#define OP_DECREMENT        ((uint8_t)(0x02))

struct hw_packet {
    uint8_t  flag;
    uint8_t  op;
    uint16_t len;
    uint32_t seq;
    uint8_t  data[BUF_SIZE];
};

void send_packet(int s, uint8_t flag, uint8_t op, uint16_t len, uint32_t seq, uint8_t *data);

int main(int argc, char * argv[]) {
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host = "localhost";
    char buf[] = "2015410012";
    int s, i;

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

    //int len = strlen(buf) + 1;
    //send(s, buf, len, 0);

    uint8_t tmp[4];
    uint32_t tmp2 = 2015410012;
    memcpy(tmp, &tmp2, sizeof(uint32_t));
    send_packet(s, FLAG_HELLO, OP_ECHO, 4, 0, tmp);

    while(1) {
        char buf[BUF_SIZE] = {};

        // struct hw_packet rcvd_packet;
        recv(s, buf, BUF_SIZE, 0);

        uint8_t flag = buf[0];
        uint8_t op = buf[1];
        uint16_t len = (buf[2] << 8) + buf[3];
        uint32_t data = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];

        printf("rcvd");
        for(i = 0; i < len + 8; i++) {
            if(i%4 == 0)
                printf("\n");
            printf("%X", buf[i]);
        }
        printf("\n\n");

        // printf("flag : %X\n", rcvd_packet.flag);
        // printf("op   : %X\n", rcvd_packet.op);
        // printf("len  : %X\n", rcvd_packet.len);
        // printf("seq  : %X\n", rcvd_packet.seq);
        //
        // printf("data : ");
        // for(i = 0; i < rcvd_packet.len; i++)
        //     printf("%02X", rcvd_packet.data[i]);
        // printf("\n\n");

        if(flag == FLAG_INSTRUCTION) {
            if(op != OP_ECHO) {
                data += (op == OP_INCREMENT) ? 1 : -1;

                for(i = 3; i >= 0; i--) {
                    buf[i+8] = (uint8_t) data;
                    data >>= 8;
                }
            }

            send(s, buf, len + 8, 0);
        }

        if(rcvd_packet.flag == FLAG_TERMINATE)
            break;
    }

    return 0;
}

void send_packet(int s, uint8_t flag, uint8_t op, uint16_t len, uint32_t seq, uint8_t *data) {
    struct hw_packet buf_struct;
    buf_struct.flag = flag;
    buf_struct.op = op;
    buf_struct.len = len;
    buf_struct.seq = seq;
    memcpy(buf_struct.data, data, len);

    printf("send flag : %X\n", buf_struct.flag);
    printf("send op   : %X\n", buf_struct.op);
    printf("send len  : %X\n", buf_struct.len);
    printf("send seq  : %X\n", buf_struct.seq);
    printf("send data : ");
    int i;
    for(i = 0; i < buf_struct.len; i++)
        printf("%02X", buf_struct.data[i]);
    printf("\n\n");


    send(s, (char*) &buf_struct, 8+buf_struct.len, 0);
}
