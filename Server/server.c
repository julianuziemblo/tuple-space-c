#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>

//? definition for libc to have kill() prototype
#define __USE_POSIX 1
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tuple.h"
#include "ts_list.h"
#include "tuple_space.h"

#define LOOP while(true)

#define SERVER_MAX_WORKERS              32
#define SERVER_WORKER_IS_ACTIVE(w)      (w->pid == 0 ? "IDLE" : "ACTIVE")
#define SERVER_MAX_HELLO_RETRANSMITS    3
// after how many packets the server prints its stats
#define SERVER_MAX_STAT_COUNTER		10

#define PIPE_READ_END                   0
#define PIPE_WRITE_END                  1

#define RETRANSMISSION_TIMEOUT_MS       5000
#define SERVER_RETRY_TIMEOUT_MS         10000

struct timeval retransmission_timeout = {
    .tv_sec = RETRANSMISSION_TIMEOUT_MS / 1000,
    .tv_usec = (RETRANSMISSION_TIMEOUT_MS % 1000) * 1000
};

tslist_t list;

typedef struct {
    pid_t pid;
    uint32_t id;
    struct sockaddr_in client_addr;
    unsigned int client_addr_len;
    int update_packet[2];
    int exit_packet[2];
} worker_t;

worker_t workers[SERVER_MAX_WORKERS];


typedef struct {
    uint32_t n_msgs_recv;
    uint32_t nout_msgs_recv;
    uint32_t np_msgs_recv;
    float avg_msg_len;
} server_stats_t;

static server_stats_t server_stats = {
    .avg_msg_len    = 0.0F,
    .n_msgs_recv    = 0,
    .nout_msgs_recv = 0,
    .np_msgs_recv   = 0
};

void print_server_stats() {
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("[SERVER] Server stats:\n");
    printf("\t-Timestamp: %ld\n", ((time.tv_sec * 1000) + (time.tv_usec / 1000)));
    printf("\t-Total messages received: %d\n", server_stats.n_msgs_recv);
    printf("\t-Total OUT messages received: %d\n", server_stats.nout_msgs_recv);
    printf("\t-Total INP and RDP messages received: %d\n", server_stats.np_msgs_recv);
    printf("\t-Average message length (size): %f\n", server_stats.avg_msg_len);
    printf("\t-");
    tslist_print(&list);
    printf("[SERVER] Workers:\n");
    for(int i=0; i< SERVER_MAX_WORKERS; i++) {
        worker_t* cw = &workers[i];
        printf("\tWorker %d: pid %d (%s)", 
            cw->id, cw->pid, 
            SERVER_WORKER_IS_ACTIVE(cw)
        );
        if(cw->pid != 0) {
            printf(", client address: %s:%d",
                inet_ntoa(cw->client_addr.sin_addr), 
                ntohs(cw->client_addr.sin_port)
            );
        } 
        printf("\n");
    }
}

struct sockaddr_in server_addr;
struct sockaddr_in* recv_addr;
int server_sockfd;

void init_workers() {
    for(int i=0; i<SERVER_MAX_WORKERS; i++) 
        workers[i] = (worker_t) {
            .pid = (pid_t)0,
            .id  = i
        };
}

//* Blocks until a worker is available. */
worker_t* get_worker(struct sockaddr_in addr, unsigned int addr_len) {
    LOOP {
        for(int i=0; i<SERVER_MAX_WORKERS; i++) {
            if(workers[i].pid == 0) {
                worker_t* worker = &workers[i];

                // set addresses
                worker->client_addr = addr;
                worker->client_addr_len = addr_len;

                // setup pipes
                pipe(worker->update_packet);
                pipe(worker->exit_packet);
                fcntl(worker->exit_packet[PIPE_READ_END], F_SETFL, O_NONBLOCK);

                return worker;
            }
        }
    }
}

void free_worker(worker_t* worker) {
    // deallocate resources 
    // place the worker into the pool (give it pid=0)
    worker->pid = 0;
    close(worker->update_packet[PIPE_READ_END]);
    close(worker->update_packet[PIPE_WRITE_END]);
    close(worker->exit_packet[PIPE_WRITE_END]);
    close(worker->exit_packet[PIPE_READ_END]);
}

void send_packet(ts_packet_t* packet, struct sockaddr_in addr, unsigned int addr_len) {

    addr.sin_port = htons(TS_PORT); //TODO: zmienić na TS_PORT

    printf("Sending back %s (%s) to client %d: %s:%d\n", ts_rtoa(packet->req_type), ts_ftoa(packet->flags), packet->num, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    printf("Sent tuple: ");
    tuple_print(&packet->tuple);

    int res = sendto(server_sockfd, packet, sizeof(*packet), 0, (struct sockaddr*)&addr, sizeof(addr));
    if(res < 0) {
        perror("send error");
    }
}

bool tuple_is_not_template(tuple_t* tuple) {
    for(int i=0; i<tuple->size; i++) {
        if(tuple->fields[i].occupied == TUPLE_OCCUPIED_NO) 
            return false;
    }
    return true;
}

ts_packet_t bad_request(ts_packet_t* packet) {
    printf("Got %s request with bad flags.\n", ts_rtoa(packet->req_type));
    return ts_empty_packet(
        packet->tuple.name,
        packet->num, 
        packet->flags | TS_FLAG_ERR
    );
}

ts_packet_t response_hello(ts_packet_t* packet) {
    if((packet->flags & TS_FLAG_HELLO) 
    || (packet->flags & TS_FLAG_KEEPALIVE)) {
        printf("Got %s message.\n", ts_ftoa(packet->flags));
        return ts_empty_packet(
            packet->tuple.name, 
            TS_ANS_ACK_NUM(packet->num), 
            packet->flags | TS_FLAG_ACK
        );
    } else {
        return bad_request(packet);
    }
}

ts_packet_t response_out(ts_packet_t packet, worker_t* worker) {
    // check if proper flags are set
    if(packet.flags != 0) {
        return bad_request(&packet);
    }

    // check if tuple is good (e.g. not a template)
    if(!tuple_is_not_template(&packet.tuple)) {
        // Send ACK + ERR (request fulfulled, tuple not found)
        printf("Got improper tuple=");
        tuple_print(&packet.tuple);

        return (ts_packet_t) {
            .req_type = TS_REQ_OUT,
            .flags    = TS_FLAG_ERR,
            .num      = TS_ANS_ACK_NUM(packet.num),
            .tuple    = tuple_new(packet.tuple.name, 0)
        };
    }

    // add requested tuple to tuple space
    // send request to server to add to tuple space
    packet.flags |= TS_FLAG_KEEPALIVE;
    write(worker->exit_packet[PIPE_WRITE_END], &packet, sizeof(packet));

    // send ANS + ACK
    return (ts_packet_t) {
        .req_type   = TS_REQ_OUT,
        .flags      = TS_FLAG_ACK,
        .num        = TS_ANS_ACK_NUM(packet.num),
        .tuple      = tuple_new(packet.tuple.name, 0)
    };
}

ts_packet_t response_nonblocking(ts_packet_t packet, worker_t* worker) {
    // check if proper flags are set
    if(packet.flags != 0) {
        return bad_request(&packet);
    }

    // send request to server to provide the tuple
    packet.flags |= TS_FLAG_KEEPALIVE;
    write(worker->exit_packet[PIPE_WRITE_END], &packet, sizeof(packet));

    // receive request from server with the tuple
    ts_packet_t response;
    //while((response.flags & TS_FLAG_ACK))
    read(worker->update_packet[PIPE_READ_END], &response, sizeof(response));

    return response;
}

ts_packet_t response_blocking(ts_packet_t packet, worker_t* worker) {
    // check if proper flags are set
    if(packet.flags != 0) {
        return bad_request(&packet);
    }

    packet.flags |= TS_FLAG_KEEPALIVE;

    LOOP {
        // re-send request on every recieved packet
        write(worker->exit_packet[PIPE_WRITE_END], &packet, sizeof(packet));
        printf("[WORKER %d] Sent packet to server: ", worker->id);
        ts_packet_print(&packet);
        ts_packet_t response;
        read(worker->update_packet[PIPE_READ_END], &response, sizeof(response));
        printf("[WORKER %d] Received packet from server: ", worker->id);
        ts_packet_print(&response);

        if((response.flags & TS_FLAG_ERR) 
            || (response.flags & TS_FLAG_ACK) 
            || (response.tuple.size == 0)) {
            // on not found: continue
            continue;
        } else {
            // on found: return 
            return response;
        }
    }
}

//* Wait for [timeout] milliseconds, if no ACK arrives - retransmit (only once). */
void handle_retransmission(ts_packet_t response, worker_t* worker, int timeout) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(worker->update_packet[PIPE_READ_END], &readfds);
    struct timeval tm = {
        .tv_sec = timeout / 1000,
        .tv_usec = (timeout % 1000) * 1000
    };
    struct timeval start_time;
    struct timeval end_time;
    gettimeofday(&start_time, NULL);
    printf("[WORKER %d] Waiting for ACK for %s request %d. Elapsed time: %ldms\n", worker->id, ts_rtoa(response.req_type), response.num, ((tm.tv_sec) * 1000 + (tm.tv_usec) / 1000));
    if(select(worker->update_packet[PIPE_READ_END] + 1, &readfds, NULL, NULL, &tm) > 0) {
        gettimeofday(&end_time, NULL);
        long int start_time_in_mill = (long int)((start_time.tv_sec) * 1000 + (start_time.tv_usec) / 1000);
        long int end_time_in_mill = (long int)((end_time.tv_sec) * 1000 + (end_time.tv_usec) / 1000);
        long int diff = end_time_in_mill - start_time_in_mill;
        // long int elapsed_time = timeout - diff;

        ts_packet_t packet;
        read(worker->update_packet[PIPE_READ_END], &packet, sizeof(packet));
        if((packet.flags & TS_FLAG_ACK) && (packet.req_type == response.req_type)) {
            printf("[WORKER %d] Got ACK from client %d. Elapsed time: %ldms. Returning...\n", worker->id, response.num, diff);
            return;
        } else {
            //if(elapsed_time > 0) {
            //    handle_retransmission(response, worker, elapsed_time);
            //}
        }
    } 
    printf("[WORKER %d] No ACK from client %d. Retransmitting and closing...\n", worker->id, response.num);
    response.flags |= TS_FLAG_RETRANSMIT;
    send_packet(&response, worker->client_addr, worker->client_addr_len);
}

ts_packet_t get_response(ts_packet_t* request, worker_t* worker) {
    switch(request->req_type) {
        case TS_REQ_EMPTY:
            return response_hello(request);
        case TS_REQ_OUT:
            return response_out(*request, worker);
        case TS_REQ_INP:
        case TS_REQ_RDP:
            return response_nonblocking(*request, worker);
        case TS_REQ_IN:
        case TS_REQ_RD:
            return response_blocking(*request, worker);
        default:
            return (ts_packet_t) {
                .flags    = request->flags | TS_FLAG_ERR,
                .num      = request->num,
                .req_type = request->req_type,
                .tuple    = tuple_new(request->tuple.name, 0)
            };
    }
}

void handle_request(ts_packet_t* request, worker_t* worker) {
    //* 0. check, if this is not parent process */
    if(worker->pid != 0) {
        printf("[WARNING] Tried to call handle_requets from parent. Returninig...\n");
        return;
    }

    //* 1. get appropriate response packet */
    ts_packet_t response = get_response(request, worker);

    response.flags |= TS_FLAG_ACK;
    //* 2. send response packet */
    send_packet(&response, worker->client_addr, worker->client_addr_len);

    //* 3. wait for retransmissions */
    // handle_retransmission(response, worker, RETRANSMISSION_TIMEOUT_MS);

    //* 4. exit */
    ts_packet_t exit_packet = {
        .flags    = 0,
        .num      = 0,
        .req_type = TS_REQ_EMPTY,
        .tuple    = tuple_new(request->tuple.name, 0)
    };
    write(worker->exit_packet[PIPE_WRITE_END], &exit_packet, sizeof(exit_packet));
    return;
}

void handle_worker_request(ts_packet_t* packet, worker_t* worker) {
    printf("Handling worker's %d (pid: %d) request %s (%s) %d with tuple \n", worker->id, worker->pid, ts_rtoa(packet->req_type), ts_ftoa(packet->flags), packet->num);
    tuple_print(&packet->tuple);
    // check if this function wasnt called from child
    if(worker->pid == 0) {
        printf("`handle_worker_request` called from child.\nAborting...\n");
        return;
    }

    uint32_t search_res;
    tuple_t search_tuple = packet->tuple;
    ts_packet_t response = {
        .flags    = 0,
        .req_type = packet->req_type
    };

    switch(packet->req_type) {
        case TS_REQ_EMPTY:
            if(packet->flags == 0 && packet->num == 0) {
                // kill(worker->pid, SIGINT);
                free_worker(worker);
            }
            return;
        case TS_REQ_OUT:
            printf("[SERVER] Adding tuple: ");
            tuple_print(&packet->tuple);
            tslist_add(&list, packet->tuple);
            server_stats.nout_msgs_recv++;
            return;
        case TS_REQ_IN:
        case TS_REQ_INP:
            printf("[SERVER] Withdrawing tuple: ");
            tuple_print(&packet->tuple);
            search_res = tslist_withdraw(&list, &search_tuple);
            if(search_res == TSLIST_NOTFND) {
                printf("[SERVER] Couldn't withdraw tuple.\n");
            } else {
                printf("[SERVER] Withdrawn tuple ");
                tuple_print(&search_tuple);
            }
            break;
        case TS_REQ_RD:
        case TS_REQ_RDP:
            printf("[SERVER] Finding tuple: ");
            tuple_print(&packet->tuple);
            search_res = tslist_find(&list, &search_tuple);
            if(search_res == TSLIST_NOTFND) {
                printf("[SERVER] Couldn't find tuple.\n");
            } else {
                printf("[SERVER] Found tuple ");
                tuple_print(&search_tuple);
            }
            break;
    }

    if(search_res == TSLIST_NOTFND) {
        // tuple not found
        response.flags |= TS_FLAG_ERR;
        response.num   = packet->num;
        response.tuple = tuple_new(packet->tuple.name, 0);
    } else {
        // tuple found
        response.num      = TS_ANS_ACK_NUM(packet->num);
        response.tuple    = search_tuple;
    }

    // write to worker
    write(worker->update_packet[PIPE_WRITE_END], &response, sizeof(response));
}

void handle_sigint(int signum) {
    printf("Caught signum: %d (2 == SIGINT)\n", signum);
    // kill all children 
    for(int i=0; i<SERVER_MAX_WORKERS; i++) {
        if(workers[i].pid != 0) {
            printf("Killing worker %d with pid %d\n", i, workers[i].pid);
            kill(workers[i].pid, SIGINT);
        }
    }

    // uninitialize list
    tslist_free(&list);
    printf("Uninitialized the list\n");

    // free all mallocated resources
    free(recv_addr);

    // close all descriptors
    close(server_sockfd);

    printf("Terminating...\n");
    exit(0);
}

int main(void) {
    // initialize list
    list = tslist_new();

    // initialize workers
    init_workers();

    // initialize interrupt handlers
    signal(SIGINT, handle_sigint);

    uint32_t stat_counter = 0;

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(TS_PORT);
    server_addr.sin_family = AF_INET;
    memset(server_addr.sin_zero, '0', sizeof(*server_addr.sin_zero));

    //* create UDP socket for listening to server1 */
    if ((server_sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    	perror("socket error");
        exit(EXIT_FAILURE);
    } printf("Socket opened\n");

    //* bind [client_recv_serv1_sockfd] to [SERV1_PORT] */
    if( bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1) {
    	perror("bind error");
        exit(EXIT_FAILURE);
    } printf("Binded socket to local port: %d\nListening...\n", TS_PORT);

    LOOP {
        stat_counter++;
	if(stat_counter >= SERVER_MAX_STAT_COUNTER) {
	    print_server_stats();
	    stat_counter = 0;
	}
        recv_addr = (struct sockaddr_in*)malloc(sizeof(*recv_addr));
        unsigned int recv_len = sizeof(recv_addr);
        ts_packet_t packet = {0};
        memset(&packet, 0, sizeof(packet));

        // get request
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_sockfd, &readfds);
        struct timeval timeout;
        timeout.tv_sec = SERVER_RETRY_TIMEOUT_MS / 1000;
        timeout.tv_usec = (SERVER_RETRY_TIMEOUT_MS % 1000) * 1000;
        printf("[SERVER] Waiting on recv...\n");
        if(select(server_sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            int req_len = recvfrom(server_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)recv_addr, &recv_len);
            printf("[SERVER] Received packet: ");
            printf("flags: %d, ", packet.flags);
            ts_packet_print(&packet);
            // give the job to `empty-handed` worker
            worker_t* worker = get_worker(*recv_addr, recv_len);

            if(req_len < 0) {
                perror("recvfrom error");
            } else {
                if(!(packet.flags & TS_FLAG_ACK)) {
                    // if the message is not an ACK: dedicate worker to handle it.
                    // update stats
                    server_stats.n_msgs_recv++;
                    server_stats.avg_msg_len = (server_stats.avg_msg_len + packet.tuple.size) / server_stats.n_msgs_recv;
                    if(packet.req_type == TS_REQ_INP || packet.req_type == TS_REQ_RDP) {
                        server_stats.np_msgs_recv++;
                    }

                    pid_t pid = fork();
                    if(pid < 0) {
                        perror("fork error");
                    } else if(pid == 0) {
                        //? CHILD
                        close(worker->update_packet[PIPE_WRITE_END]);
                        close(worker->exit_packet[PIPE_READ_END]);

                        handle_request(&packet, worker);
                        printf("[WORKER %d] Exiting process...", worker->id);
                        exit(0);
                    } else {
                        //? PARENT
                        worker->pid = pid;
                        close(worker->update_packet[PIPE_READ_END]);
                        close(worker->exit_packet[PIPE_WRITE_END]);

                        sleep(1);

                        printf("[SERVER] Worker %d (pid: %d) will handle packet %s (%s) num %d tuple ", worker->id, worker->pid, ts_rtoa(packet.req_type), ts_ftoa(packet.flags), packet.num);
                        tuple_print(&packet.tuple);
                    }
                } else {
                    // if ACK: send the ACK to all active workers
                    free_worker(worker);
		            printf("[SERVER] Got ACK from client %s:%d\n", inet_ntoa(recv_addr->sin_addr), ntohs(recv_addr->sin_port));
                    //for(int i=0; i<SERVER_MAX_WORKERS; i++) {
                    //   if(workers[i].pid != 0) {
                    //       write(workers[i].update_packet[PIPE_WRITE_END], &packet, sizeof(packet));
                    //   }
                    //}
                }
            }
        } else {
            // select timeout
            printf("[SERVER] Server recv timeout.\n");
            print_server_stats();
        }

        // check workers
        for(int i=0; i<SERVER_MAX_WORKERS; i++) {
            // cw == current worker
            worker_t* cw = &workers[i];
            if(cw->pid != 0) {
                // check for worker requests and handle them
                ts_packet_t worker_request;
                uint32_t resp_num = read(cw->exit_packet[PIPE_READ_END], &worker_request, sizeof(worker_request));
                //* flags KEEPALIVE: not end, handle request */
                //* no flags: kill the child, free the worker */
                if(resp_num != -1) {
                    if(worker_request.flags == 0 && worker_request.num == 0) {
                        printf("[SERVER] Killing worker %d with pid %d\n", i, cw->pid);
                        // kill(cw->pid, SIGINT);
			            free_worker(cw);
                    } else {
                        handle_worker_request(&worker_request, cw);
                    }
                }
            }
        }

        free(recv_addr);
    }

    return 0;
}
