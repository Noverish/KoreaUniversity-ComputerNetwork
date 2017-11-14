#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#define SERVER_PORT 47500
#define MAX_LINE 256

#define FLAG_HELLO		((uint8_t)(0x01 << 7))
#define FLAG_INSTRUCTION	((uint8_t)(0x01 << 6))
#define FLAG_RESPONSE		((uint8_t)(0x01 << 5))
#define FLAG_TERMINATE		((uint8_t)(0x01 << 4))

#define OP_ECHO			((uint8_t)(0x00))
#define OP_INCREMENT		((uint8_t)(0x01))
#define OP_DECREMENT		((uint8_t)(0x02))

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
    char buf[] = "2015410012";
    int s;

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
        struct hw_packet rcvd_packet;
        recv(s, (char*) &rcvd_packet, sizeof(struct hw_packet), 0);

        printf("rcvd flag : %X\n", rcvd_packet.flag);
        printf("rcvd op   : %X\n", rcvd_packet.operation);
        printf("rcvd len  : %X\n", rcvd_packet.data_len);
        printf("rcvd seq  : %X\n", rcvd_packet.seq_num);
        printf("rcvd data : ");
        int i;
        for(i = 0; i < rcvd_packet.data_len; i++)
            printf("%02X", rcvd_packet.data[i]);
        printf("\n\n");

        if(rcvd_packet.flag == FLAG_INSTRUCTION) {

            if(rcvd_packet.operation != OP_ECHO) {
                uint32_t tmp;
                memcpy(&tmp, rcvd_packet.data, sizeof(uint32_t));

                tmp += (rcvd_packet.operation == OP_INCREMENT) ? 1 : -1;

                memcpy(rcvd_packet.data, &tmp, sizeof(uint32_t));
            }

            send_packet(s, FLAG_RESPONSE, rcvd_packet.operation, rcvd_packet.data_len, rcvd_packet.seq_num, rcvd_packet.data);
        }

        if(rcvd_packet.flag == FLAG_TERMINATE)
            break;
    }

    return 0;
}

void send_packet(int s, uint8_t flag, uint8_t op, uint16_t len, uint32_t seq, uint8_t *data) {
    struct hw_packet buf_struct;
    buf_struct.flag = flag;
    buf_struct.operation = op;
    buf_struct.data_len = len;
    buf_struct.seq_num = seq;
    memcpy(buf_struct.data, data, len);

    printf("send flag : %X\n", buf_struct.flag);
    printf("send op   : %X\n", buf_struct.operation);
    printf("send len  : %X\n", buf_struct.data_len);
    printf("send seq  : %X\n", buf_struct.seq_num);
    printf("send data : ");
    int i;
    for(i = 0; i < buf_struct.data_len; i++)
        printf("%02X", buf_struct.data[i]);
    printf("\n\n");


    send(s, (char*) &buf_struct, sizeof(struct hw_packet), 0);
}
