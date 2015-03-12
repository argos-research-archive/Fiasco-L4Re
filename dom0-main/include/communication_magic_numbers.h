#pragma once

//these numbers are used in network communication
//to identify the type of the payload

//client wants to send control messages
#define CONTROL 0xC047201
//client wants to send a LUA command
#define LUA 0x10A

//Possible answers to LUA commands
#define LUA_OK 0x10A900D
#define LUA_ERROR 0x10ABAD

//Control messages:
//client wants to send a binary
#define SEND_BINARY 0xF11E
//server tells client that he's ready to receive
#define GO_SEND 0x90
//server tells client that he received the binary
#define OK_RECEIVED 0x900D

//#define CLOSE 0xC105E
//#define CLOSE2 0xC105E2
