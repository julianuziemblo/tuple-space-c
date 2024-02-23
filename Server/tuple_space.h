#ifndef _TUPLE_SPACE_H
#define _TUPLE_SPACE_H
#include "tuple.h"
#include <stdlib.h>

// TODO: Change it to real IP
#define TS_SERVER_IP "192.168.56.101"
#define TS_PORT 2137

#define TS_SUCCESS 0

#define TS_ENOMTCH 1
#define TS_ENOMTCH_STR "No matching tuple found";
#define TS_EUDPERR 2
#define TS_EUDPERR_STR "UDP error";
#define TS_ETMOUT  3
#define TS_ETMOUT_STR "No response from the server";

#define TS_REQ_EMPTY            0b000
#define TS_REQ_EMPTY_STR        "EMPTY"
#define TS_REQ_OUT              0b001
#define TS_REQ_OUT_STR          "OUT"
#define TS_REQ_IN               0b010
#define TS_REQ_IN_STR           "IN"
#define TS_REQ_INP              0b011
#define TS_REQ_INP_STR          "INP"
#define TS_REQ_RD               0b100
#define TS_REQ_RD_STR           "RD"
#define TS_REQ_RDP              0b101
#define TS_REQ_RDP_STR          "RDP"

#define TS_FLAG_ACK             0b00001
#define TS_FLAG_ACK_STR         "ACK"
#define TS_FLAG_RETRANSMIT      0b00010
#define TS_FLAG_RETRANSMIT_STR  "RETRANSMIT"
#define TS_FLAG_KEEPALIVE       0b00100
#define TS_FLAG_KEEPALIVE_STR   "KEEPALIVE"
#define TS_FLAG_HELLO           0b01000
#define TS_FLAG_HELLO_STR       "HELLO"
#define TS_FLAG_ERR             0b10000
#define TS_FLAG_ERR_STR         "ERROR"

#define _TS_REQ_TYPE_BITS       3
#define _TS_FLAG_BITS           5
#define _TS_NUM_BITS            24

#define TS_ACK_NUM              (rand() % (1 << _TS_NUM_BITS))
#define TS_ANS_ACK_NUM(num)     ((num + 1) % (1 << _TS_NUM_BITS))

typedef struct {
    uint32_t req_type:  _TS_REQ_TYPE_BITS;
    uint32_t flags:      _TS_FLAG_BITS;
    uint32_t num:       _TS_NUM_BITS;
    tuple_t  tuple;
} ts_packet_t;

//* Create empty message. */
ts_packet_t ts_empty_packet(char* name, uint32_t num, uint32_t flags);

//* Add [tuple] to the tuple space. */
uint32_t ts_out(tuple_t tuple);

// !BLOCKING!
//* Withdraw a tuple matching the [tuple_template] */
//* from the tuple space into the [tuple_template] */
uint32_t ts_in(tuple_t* tuple_template);

//* Try to withdraw a tuple matching the [tuple_template] */
//* from the tuple space into the [tuple_template] */
//* Return value is 0 on success and non-zero on failure. */
uint32_t ts_inp(tuple_t* tuple_template);

// !BLOCKING!
//* Read a tuple matching the [tuple_template] */
//* from the tuple space into the [tuple_template] */
uint32_t ts_rd(tuple_t* tuple_template);

//* Try to read a tuple matching the [tuple_template] */
//* from the tuple space into the [tuple_template] */
//* Return value is 0 on success and non-zero on failure. */
uint32_t ts_rdp(tuple_t* tuple_template);

/* Get error message from TS error code. */
const char* ts_etoa(uint32_t errnum);

//* Get flags name from flags code. */
const char* ts_ftoa(uint32_t flags);

//* Get request type name from request type code. */
const char* ts_rtoa(uint32_t req_type);

//* Print given packet. */
void ts_packet_print(ts_packet_t* packet);

#endif
