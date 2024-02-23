#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "tuple.h"
#include "tuple_space.h"

#define SIN_ZERO_DEFAULT "00000000"

const tuple_field_t tuple_empty_field = {
    .occupied = TUPLE_OCCUPIED_NO,
    .tuple_type = TUPLE_TYPE_UNDEF,
    .data = {
        ._int = 0
    }
};

const struct sockaddr_in _ts_server_addr() {
    return (const struct sockaddr_in) {
        .sin_addr.s_addr = inet_addr(TS_SERVER_IP),
        .sin_port        = htons(TS_PORT),
        .sin_family      = AF_INET,
        .sin_zero        = SIN_ZERO_DEFAULT
    };
}

const struct sockaddr_in _ts_self_addr() {
    return (const struct sockaddr_in) {
        .sin_addr.s_addr =  INADDR_ANY,
        .sin_port = htons(TS_PORT), // TODO: change to TS_PORT
        .sin_family = AF_INET,
        .sin_zero   = SIN_ZERO_DEFAULT
    };
}

//* Create empty message. */
ts_packet_t ts_empty_packet(char* name, uint32_t num, uint32_t flags) {
    assert(strlen(name) >= 0 && strlen(name) <= TUPLE_NAME_SIZE);

    tuple_field_t fields[TUPLE_MAX_SIZE];

    for(int i=0; i<TUPLE_MAX_SIZE; i++) {
        fields[i] = tuple_empty_field;
    }

    tuple_t tuple = {
        .size = 0
    };
    strncpy(tuple.name, name, sizeof(tuple.name));

    memcpy(tuple.fields, fields, sizeof(fields));

    return (ts_packet_t) {
        .req_type   = TS_REQ_EMPTY,
        .flags       = flags,
        .num        = num,
        .tuple      = tuple
    };
}

int _ts_init_socket() {
    int sockfd;
    const struct sockaddr_in self_addr = _ts_self_addr();
    //* create UDP socket for listening to server1 */
    if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    	perror("socket error");
        return -1;
    } printf("Socket opened\n");

    //* bind [client_recv_serv1_sockfd] to [SERV1_PORT] */
    if( bind(sockfd, (struct sockaddr*)&self_addr, sizeof(self_addr) ) == -1) {
    	perror("bind error");
        return -1;
    } printf("Binded socket to local port: %d\nListening...\n", ntohs(self_addr.sin_port));

    return sockfd;
}

uint32_t _ts_send_packet(
    uint32_t req_type, 
    uint32_t flags, 
    tuple_t* tuple, 
    const struct sockaddr_in* addr, 
    int sockfd
) {
    ts_packet_t request = {
        .flags    = flags,
        .num      = TS_ACK_NUM,
        .req_type = req_type,
        .tuple    = *tuple
    };

    printf("Sent packet: ");
    ts_packet_print(&request);

    return sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr*)addr, sizeof(*addr));
}

ts_packet_t _ts_recv_packet(int sockfd) {
    ts_packet_t response;
    memset(&response, 0, sizeof(response));
    struct sockaddr_in* recv_addr = (struct sockaddr_in*)malloc(sizeof(*recv_addr));
    unsigned int recv_len = 0;
    int recv_res = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr*)recv_addr, &recv_len);
    if(recv_res < 0) {
        printf("Received from addr of len %d\n", recv_len);
        perror("recvfrom error");
    } else {
        printf("Received packet: ");
        ts_packet_print(&response);
    }
    free(recv_addr);
    return response;
}

//* Add [tuple] to the tuple space. */
uint32_t ts_out(tuple_t tuple) {
    const struct sockaddr_in server_addr = _ts_server_addr();
    int sockfd = _ts_init_socket();
    const uint32_t req_type = TS_REQ_OUT;
    const uint32_t flags    = 0;

    // Send request
    int send_res = _ts_send_packet(req_type, flags, &tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_out` function");
        close(sockfd);
        return send_res;
    } 

    // Wait for response
    ts_packet_t response = _ts_recv_packet(sockfd);

    // Send ACK
    tuple_t empty_tuple = tuple_new(tuple.name, 0);
    send_res = _ts_send_packet(req_type, TS_FLAG_ACK, &empty_tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_out` function (while sending ACK)");
        close(sockfd);
        return send_res;
    }

    // Close file descriptors
    close(sockfd);

    // Return status
    if(!(response.flags & TS_FLAG_ERR)) {
        return TS_SUCCESS;
    } else {
        return -1;
    }
}   

// !BLOCKING!
/* Withdraw a tuple matching the [tuple_template] */
/* from the tuple space into the [tuple_template] */
uint32_t ts_in(tuple_t* tuple_template) {
    const struct sockaddr_in server_addr = _ts_server_addr();
    const uint32_t req_type = TS_REQ_IN;
    int sockfd = _ts_init_socket();

    // Send request
    int send_res = _ts_send_packet(req_type, 0, tuple_template, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_in` function");
        close(sockfd);
        return send_res;
    }

    // Wait for response
    ts_packet_t response = _ts_recv_packet(sockfd);
    *tuple_template = response.tuple;

    // Send ACK
    tuple_t empty_tuple = tuple_new(tuple_template->name, 0);
    send_res = _ts_send_packet(req_type, TS_FLAG_ACK, &empty_tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_in` function (while sending ACK)");
        close(sockfd);
        return send_res;
    }

    // Close file descriptors
    close(sockfd);

    // Return status
    if(!(response.flags & TS_FLAG_ERR)) {
        return TS_SUCCESS;
    } else {
        return -1;
    }
}

/* Try to withdraw a tuple matching the [tuple_template] */
/* from the tuple space into the [tuple_template]. */
/* Return value is 0 on success and non-zero on failure. */
uint32_t ts_inp(tuple_t* tuple_template) {
    const struct sockaddr_in server_addr = _ts_server_addr();
    int sockfd = _ts_init_socket();
    const uint32_t req_type = TS_REQ_INP;

    // Send request
    int send_res = _ts_send_packet(req_type, 0, tuple_template, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_inp` function");
        close(sockfd);
        return send_res;
    }

    // Wait for response
    ts_packet_t response = _ts_recv_packet(sockfd);
    *tuple_template = response.tuple;

    // Send ACK
    tuple_t empty_tuple = tuple_new(tuple_template->name, 0);
    send_res = _ts_send_packet(req_type, TS_FLAG_ACK, &empty_tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_inp` function (while sending ACK)");
        close(sockfd);
        return send_res;
    }

    // Close file descriptors
    close(sockfd);

    // Return status
    if((response.flags & TS_FLAG_ERR)
        && (response.flags & TS_FLAG_ACK)) {
        return TS_ENOMTCH;
    } else if(response.flags & TS_FLAG_ACK) {
        return TS_SUCCESS;
    } else {
        return -1;
    }
}

// !BLOCKING!
/* Read a tuple matching the [tuple_template] */
/* from the tuple space into the [tuple_template] */
uint32_t ts_rd(tuple_t* tuple_template) {
    const struct sockaddr_in server_addr = _ts_server_addr();
    int sockfd = _ts_init_socket();
    const uint32_t req_type = TS_REQ_RD;

    // Send request
    int send_res = _ts_send_packet(req_type, 0, tuple_template, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_rd` function");
        close(sockfd);
        return send_res;
    }

    // Wait for response
    ts_packet_t response = _ts_recv_packet(sockfd);
    *tuple_template = response.tuple;

    // Send ACK
    tuple_t empty_tuple = tuple_new(tuple_template->name, 0);
    send_res = _ts_send_packet(req_type, TS_FLAG_ACK, &empty_tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_rd` function (while sending ACK)");
        close(sockfd);
        return send_res;
    }

    // Close file descriptors
    close(sockfd);

    // Return status
    if(!(response.flags & TS_FLAG_ERR)) {
        return TS_SUCCESS;
    } else {
        return -1;
    }
}

//* Try to read a tuple matching the [tuple_template] */
//* from the tuple space into the [tuple_template] */
//* Return value is 0 on success and non-zero on failure. */
uint32_t ts_rdp(tuple_t* tuple_template) {
const struct sockaddr_in server_addr = _ts_server_addr();
    int sockfd = _ts_init_socket();
    const uint32_t req_type = TS_REQ_RDP;

    // Send request
    int send_res = _ts_send_packet(req_type, 0, tuple_template, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_rdp` function");
        close(sockfd);
        return send_res;
    }

    // Wait for response
    ts_packet_t response = _ts_recv_packet(sockfd);
    *tuple_template = response.tuple;

    // Send ACK
    tuple_t empty_tuple = tuple_new(tuple_template->name, 0);
    send_res = _ts_send_packet(req_type, TS_FLAG_ACK, &empty_tuple, &server_addr, sockfd);
    if(send_res < 0) {
        perror("send error in `ts_rdp` function (while sending ACK)");
        close(sockfd);
        return send_res;
    }

    // Close file descriptors
    close(sockfd);

    // Return status
    if((response.flags & TS_FLAG_ERR)
        && (response.flags & TS_FLAG_ACK)) {
        return TS_ENOMTCH;
    } else if(response.flags & TS_FLAG_ACK) {
        return TS_SUCCESS;
    } else {
        return -1;
    }
}

char _ts_ftoa_buf[256];
//* Get flags name from flags code. */
const char* ts_ftoa(uint32_t flags) {
    _ts_ftoa_buf[0] = '\0';
    if(flags & TS_FLAG_ACK) {
        if (strlen(_ts_ftoa_buf) != 0)
            strcat(_ts_ftoa_buf, " | ");
        strcat(_ts_ftoa_buf, TS_FLAG_ACK_STR);
    }
    if(flags & TS_FLAG_HELLO) {
        if (strlen(_ts_ftoa_buf) != 0)
            strcat(_ts_ftoa_buf, " | ");
        strcat(_ts_ftoa_buf, TS_FLAG_HELLO_STR);
    }
    if(flags & TS_FLAG_RETRANSMIT) {
        if (strlen(_ts_ftoa_buf) != 0)
            strcat(_ts_ftoa_buf, " | ");
        strcat(_ts_ftoa_buf, TS_FLAG_RETRANSMIT_STR);
    }
    if(flags & TS_FLAG_KEEPALIVE) {
        if (strlen(_ts_ftoa_buf) != 0)
            strcat(_ts_ftoa_buf, " | ");
        strcat(_ts_ftoa_buf, TS_FLAG_KEEPALIVE_STR);
    }
    if(flags & TS_FLAG_ERR) {
        if (strlen(_ts_ftoa_buf) != 0)
            strcat(_ts_ftoa_buf, " | ");
        strcat(_ts_ftoa_buf, TS_FLAG_ERR_STR);
    }
    if (strlen(_ts_ftoa_buf) == 0) {
        if (flags == 0) return "None";
        return "unknown";
    }
    return _ts_ftoa_buf;
}

//* Get request type name from request type code. */
const char* ts_rtoa(uint32_t req_type) {
    if(req_type == TS_REQ_EMPTY) return TS_REQ_EMPTY_STR;
    if(req_type == TS_REQ_IN) return TS_REQ_IN_STR;
    if(req_type == TS_REQ_INP) return TS_REQ_INP_STR;
    if(req_type == TS_REQ_OUT) return TS_REQ_OUT_STR;
    if(req_type == TS_REQ_RD) return TS_REQ_RD_STR;
    if(req_type == TS_REQ_RDP) return TS_REQ_RDP_STR;

    return "unknown";
}

//* Print given packet. */
void ts_packet_print(ts_packet_t* packet) {
    printf("PACKET {type: %s, flags: (%d)(%s), num %d, tuple: ", ts_rtoa(packet->req_type), packet->flags, ts_ftoa(packet->flags), packet->num);
    tuple_print(&packet->tuple);
}
