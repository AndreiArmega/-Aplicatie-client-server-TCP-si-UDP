#ifndef PACKETS
#define PACKETS

// structura pentru hello, cand incepe comunicatia tcp
struct tcp_hello {
    char name[51];
} __attribute__((packed));

//structura pentru subscribe, cand un client se aboneaza la un topic(sau dezaboneaza)
struct tcp_subscribe {
    char type;
    char topic[51];
} __attribute__((packed));


#define MAX_TOPIC_LENGTH 51
#define MAX_CONTENT_LENGTH 1500

// udp_message este structura pentru mesajele trimise prin udp
typedef struct __attribute__((packed)) udp_message {
    char topic[MAX_TOPIC_LENGTH + 1];  // +1 for null terminator
    unsigned char tip_date;
    char continut[MAX_CONTENT_LENGTH + 1];  // +1 for null terminator
} udp_message;

#endif