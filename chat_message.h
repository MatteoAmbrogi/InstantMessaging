/*
Lib

*/

// include
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>

//define
#define SERVER_PORT             4242
#define IND                     "127.0.0.1"
#define USERNAME_LENGTH         32
#define TIMESTAMP_LENGTH        25
#define BUFSIZE                 1024
#define CHUNKSIZE               256

//message types
#define MSG_ERR 		        0
#define MSG_END 		        5

#define MSG_OK			        10

#define MSG_ACK_STORE	        11
#define MSG_ACK_SEND 	        12

#define MSG_SIGNUP		        20
#define MSG_IN			        22

#define MSG_TEXT                32

#define MSG_ONLINELIST          34
#define MSG_ONLINELIST_N        35

#define MSG_HANGING             44
#define MSG_HANGING_RESPONSE    45

#define MSG_SHOW                50
#define MSG_SHOW_RESPONSE       51

#define MSG_ACK_READ            55

#define MSG_FILE                60
#define MSG_PORT_REQ            61


//typedef
typedef int msg_type;

// strutture per liste e file
// lista server
typedef struct device_s {
    char username[USERNAME_LENGTH];
    int port;
    int fd;
    char timestampIN[TIMESTAMP_LENGTH];
    char timestampOUT[TIMESTAMP_LENGTH];
    struct device_s *next;
} device;

// lista client device connessi
typedef struct deviceConnected_s {
    char username[USERNAME_LENGTH];
    int port;
    int fd;
    bool activeChat;
    struct deviceConnected_s *next;
} deviceConnected;

// file cronologia messaggi
typedef struct chatMessage_s {
    bool inviato;
    bool recapitato;
    char text[BUFSIZE];
} chatMessage;

// file messaggi hanging
typedef struct hangingMessage_s {
    char userSource[USERNAME_LENGTH];
    char timestamp[TIMESTAMP_LENGTH];
    char text[BUFSIZE];
} hangingMessage;



//message formats

typedef struct pk_type_s {
    msg_type mt;
} pk_type;

typedef struct pk_info_s {
    msg_type mt;
    char info[BUFSIZE];
} pk_info;

typedef struct pk_signup_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
    char password[USERNAME_LENGTH];
} pk_signup;

typedef struct pk_in_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
    char password[USERNAME_LENGTH];
    int port;
} pk_in;

typedef struct pk_text_s {
    msg_type mt;
    char userSource[USERNAME_LENGTH];
    int portSource;
    char userDest[USERNAME_LENGTH];
    int portDest;
    char text[BUFSIZE];
} pk_text;

typedef struct pk_server_ack_s {
    msg_type mt;
    int portDest;
} pk_server_ack;

typedef struct pk_onlinelist_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
} pk_onlinelist;

typedef struct pk_onlinelist_number_s {
    msg_type mt;
    int number;
} pk_onlinelist_number;

typedef struct pk_hanging_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
    int number;
    char timestamp[TIMESTAMP_LENGTH];
} pk_hanging;

typedef struct pk_show_s {
    msg_type mt;
    char userRequest[USERNAME_LENGTH];
    char userHanging[USERNAME_LENGTH];
} pk_show;

typedef struct pk_show_response_s {
    msg_type mt;
    char text[BUFSIZE];
} pk_show_response;

typedef struct pk_ack_read_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
} pk_ack_read;

typedef struct pk_file_s {
    msg_type mt;
    char userSource[USERNAME_LENGTH];
    int portSource;
    char fileName[USERNAME_LENGTH];
    unsigned long nChunk;
} pk_file;

typedef struct pk_port_req_s {
    msg_type mt;
    char username[USERNAME_LENGTH];
} pk_port_req;