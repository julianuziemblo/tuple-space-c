#include <Arduino.h>
#include <ZsutFeatures.h>
#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>

byte mac[]={0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x1f};

#define TUPLE_MAX_SIZE   4
#define TUPLE_NAME_SIZE  32

#define TUPLE_TYPE_UNDEF 0b000
#define TUPLE_TYPE_UNDEF_STR "undefined"
#define TUPLE_TYPE_INT   0b001
#define TUPLE_TYPE_INT_STR "int"
#define TUPLE_TYPE_FLOAT 0b010
#define TUPLE_TYPE_FLOAT_STR "float"

#define TUPLE_OCCUPIED_YES    0b1
#define TUPLE_OCCUPIED_NO     0b0

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

typedef union {
    int32_t _int;
    float _float;
} _tuple_data_t;

typedef struct {
    uint32_t occupied:   1;
    uint32_t tuple_type: 3;
    uint32_t padding:    28;
    _tuple_data_t data;
} tuple_field_t;

typedef struct {
    char name[TUPLE_NAME_SIZE];
    uint32_t size;
    tuple_field_t fields[TUPLE_MAX_SIZE];
} tuple_t;

typedef struct {
    uint32_t req_type:  _TS_REQ_TYPE_BITS;
    uint32_t flags:     _TS_FLAG_BITS;
    uint32_t num:       _TS_NUM_BITS;
    tuple_t  tuple;
} ts_packet_t;

#define N 10

int count;
bool results[N];
int index;
bool shouldPrint;

unsigned int port = 2137; 

// TODO: Change the address
ZsutIPAddress ts_ip = ZsutIPAddress(192,168,56,101);

ZsutEthernetUDP Udp;

void print_results() {
  for(int i=0; i<N; i++) {
    Serial.print(i);
    Serial.print(F(": "));
    if(results[i]) {
      Serial.println(F("is prime"));
    } else {
      Serial.println(F("is not prime"));
    }
    delay(1000);
  }
}

void print_packet_one_line(ts_packet_t packet) {
  Serial.print(F("{ rt: "));
  Serial.print(packet.req_type);
  Serial.print(F(", fl("));
  Serial.print(packet.flags);
  Serial.print(F("), num="));
  Serial.print(packet.num);
  Serial.print(F(", `"));
  Serial.print(packet.tuple.name);
  Serial.print(F("`, size="));
  Serial.print(packet.tuple.size);
  Serial.print(F(": "));
  for(int i=0; i<packet.tuple.size; i++) {
    tuple_field_t* field = &packet.tuple.fields[i];
    if(field->tuple_type == TUPLE_TYPE_INT) {
      Serial.print(F("(int "));
      if(field->occupied == TUPLE_OCCUPIED_YES) {
        Serial.print(field->data._int);
        Serial.print(F("), "));
      } else {
        Serial.print("?), ");
      }
    } else if(field->tuple_type == TUPLE_TYPE_FLOAT) {
      Serial.print(F("(float "));
      if(field->occupied == TUPLE_OCCUPIED_YES) {
        Serial.print(field->data._float);
        Serial.print(F("), "));
      } else {
        Serial.print("?), ");
      }
    } else {
      Serial.print("(undefined), ");
    }
  }
  Serial.println(F(" }"));
}

void out(tuple_t tuple) {
  // procedure:
  // 1. send OUT request to server with tuple
//  Serial.print(F("[OUT] Sending OUT request with tuple: ("));
//  Serial.print(tuple.name);
//  Serial.print(F(", "));
//  Serial.print(tuple.fields[0].data._int);
//  Serial.println(F(")"));
  
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);  
  ts_packet_t request;
  request.req_type = TS_REQ_OUT;
  request.flags    = 0;
  request.num      = random(16777216);
  request.tuple    = tuple;
  Udp.write((uint8_t*)&request, sizeof(request)); 
  Serial.print(F("[OUT] Sent packet to server: "));
  print_packet_one_line(request);
//  Serial.println(F("[OUT] Sent packet: "));
//  print_packet(request);
  Udp.endPacket();
  Udp.stop();
  
  // 2. wait for server answer
  Udp.begin(port);
  int packetSize = Udp.parsePacket();
  while(packetSize == 0) packetSize=Udp.parsePacket();
  ts_packet_t response;
  int resp = Udp.read((uint8_t*)&response, sizeof(response));
//  Serial.print(F("[OUT] Got response from server: "));
//  print_packet(response);
  Udp.stop();
  
  // 3. send ACK
//  Serial.println(F("[OUT] Sending ACK to server."));
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);  
  tuple_t ack_tuple;
  strcpy(ack_tuple.name, tuple.name);
  ack_tuple.size = 0;
  ts_packet_t ack;
  ack.req_type = TS_REQ_OUT;
  ack.flags    = TS_FLAG_ACK;
  ack.num      = random(16777216);
  ack.tuple    = ack_tuple;
  Udp.write((uint8_t*)&ack, sizeof(ack)); 
//  Serial.println(F("[OUT] Sent packet: "));
//  print_packet(ack);
  Udp.endPacket();
  Udp.stop();
}

bool inp(tuple_t* temp) {
  // procedure:
  // 1. send IN request to server with tuple template
//  Serial.print(F("[IN] Sending IN request with tuple: ("));
//  Serial.print(temp->name);
//  Serial.println(F(", ?)"));
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);  
  ts_packet_t request;
  request.req_type = TS_REQ_INP;
  request.flags    = 0;
  request.num      = random(16777216);
  request.tuple    = *temp;
  Udp.write((uint8_t*)&request, sizeof(request)); 
  Udp.endPacket();
  Udp.stop();
  
  // 2. wait for server answer
  Udp.begin(port);
  int packetSize = Udp.parsePacket();
  while(packetSize == 0) {
    // Serial.println(F("[IN] Waiting for packet..."));
    packetSize=Udp.parsePacket();
  }
  ts_packet_t response;
  int resp = Udp.read((uint8_t*)&response, sizeof(response));
  Serial.print(F("[INP] Got back response: "));
  print_packet_one_line(response);
//  Serial.print(F("[IN] Got response from server: "));
//  print_packet(response);
  *temp = response.tuple;
  Udp.stop();
  
  // 3. send ACK
//  Serial.println(F("[IN] Sending ACK."));
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);  
  tuple_t ack_tuple;
  strcpy(ack_tuple.name, temp->name);
  ack_tuple.size = 0;
  ts_packet_t ack;
  ack.req_type = TS_REQ_INP;
  ack.flags    = TS_FLAG_ACK;
  ack.num      = random(16777216);
  ack.tuple    = ack_tuple;
  Udp.write((uint8_t*)&ack, sizeof(ack)); 
  Udp.endPacket();
  Udp.stop();

  return (response.flags & TS_FLAG_ERR) != 0 ? false : true;
}

void print_packet(ts_packet_t packet) {
  Serial.print(F("\tPacket request type: "));
  Serial.println(packet.req_type);
  Serial.print(F("\tPacket flags: "));
  Serial.println(packet.flags);
  Serial.print(F("\tPacket number: "));
  Serial.println(packet.num);
  Serial.print(F("\tPacket tuple name: "));
  Serial.println(packet.tuple.name);
  Serial.print(F("\tPacket tuple size: "));
  Serial.println(packet.tuple.size);
  for(int i=0; i<packet.tuple.size; i++) {
    tuple_field_t* field = &packet.tuple.fields[i];
    if(field->tuple_type == TUPLE_TYPE_INT) {
      Serial.print(F("\t\t(int "));
      if(field->occupied == TUPLE_OCCUPIED_YES) {
        Serial.print(field->data._int);
        Serial.println(F(")"));
      } else {
        Serial.println("?)");
      }
    } else if(field->tuple_type == TUPLE_TYPE_FLOAT) {
      Serial.print(F("(float "));
      if(field->occupied == TUPLE_OCCUPIED_YES) {
        Serial.print(field->data._float);
        Serial.println(F(")"));
      } else {
        Serial.println("?)");
      }
    } else {
      Serial.println("\t\tundefined");
    }
  }
}

void setup() {
  randomSeed(port);
  count = 2;
  index = 2;
  results[0] = false;
  results[1] = false;
  shouldPrint = true;
  Serial.begin(115200);

  ZsutEthernet.begin(mac);

  // Send HELLO to server to see if it's active
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);
  tuple_t tuple;
  strcpy(tuple.name, "hello-from-master");
  tuple.size = 0;
  ts_packet_t packet;
  packet.req_type = TS_REQ_EMPTY;
  packet.flags    = TS_FLAG_HELLO;
  packet.num      = random(16777216);
  packet.tuple    = tuple;
  Udp.write((uint8_t*)&packet, sizeof(packet)); 
  Udp.endPacket();
  Udp.stop();

  Udp.begin(port);
  int packetSize = Udp.parsePacket();
  while(packetSize == 0) {
    // Serial.println(F("[SETUP] Waiting for HELLO response..."));
    packetSize=Udp.parsePacket();
  }
  ts_packet_t response;
  int resp = Udp.read((uint8_t*)&response, sizeof(response));
  // Serial.print(F("[SETUP] Got HELLO response from server: "));
  print_packet(response);
  Udp.stop();

  // Serial.println(F("[SETUP] Sending ACK."));
  Udp.begin(port);
  Udp.beginPacket(ts_ip, port);
  packet.flags = packet.flags | TS_FLAG_ACK;
  Udp.write((uint8_t*)&packet, sizeof(packet)); 
  Udp.endPacket();
  Udp.stop();
  // Serial.println(F("[SETUP] Sent ACK."));
}

void loop() {
  Serial.print(F("VARIABLES: count="));
  Serial.print(count);
  Serial.print(F(", index="));
  Serial.println(index);
  if(shouldPrint){
    if(count < N) {
      // first: out all the tuples to TS
      tuple_t tuple;
      strcpy(tuple.name, "check_is_prime");
      tuple.size = 1;
      tuple.fields[0].occupied   = TUPLE_OCCUPIED_YES;
      tuple.fields[0].tuple_type = TUPLE_TYPE_INT;
      tuple.fields[0].data._int  = count;
      Serial.print(F("Sending tuples. Now checking number: "));
      Serial.println(tuple.fields[0].data._int);
      out(tuple);
      count++;
    } else {
      // second: collect the results from TS
      tuple_field_t empty_field;
      empty_field.occupied = TUPLE_OCCUPIED_NO;
      empty_field.tuple_type = TUPLE_TYPE_INT;
      
      tuple_t temp_is_prime;
      strcpy(temp_is_prime.name, "is_prime");
      temp_is_prime.size = 1;
      temp_is_prime.fields[0] = empty_field;
      
      tuple_t temp_is_not_prime;
      strcpy(temp_is_not_prime.name, "is_not_prime");
      temp_is_not_prime.size = 1;
      temp_is_not_prime.fields[0] = empty_field;
  
      if(inp(&temp_is_prime)) {
        results[temp_is_prime.fields[0].data._int] = true;
        index++;
      }
  
      if(inp(&temp_is_not_prime)) {
        results[temp_is_not_prime.fields[0].data._int] = false;
        index++;
      }
    }
  }

  if(index >= N && shouldPrint) {
    Serial.println(F("Got results: "));
    print_results();
    shouldPrint = false;
  }
  
  
  delay(1000);
}
