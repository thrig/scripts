/* dns-server-version - look up DNS server version */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <err.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define DNS_HDR_LEN 12

#define REQLEN 30
char query[REQLEN] = {0xFF, 0xFF, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x07, 0x76, 0x65, 0x72,
                      0x73, 0x69, 0x6f, 0x6e, 0x04, 0x62, 0x69, 0x6e,
                      0x64, 0x00, 0x00, 0x10, 0x00, 0x03};

// some systems might want AI_ALL in flags list?
struct addrinfo hints = {.ai_family   = AF_UNSPEC,
                         .ai_flags    = AI_ADDRCONFIG,
                         .ai_socktype = SOCK_DGRAM};

// DNS replies could be larger but the version information is
// usually short (if not would be an amplification attack vector?)
unsigned char response[512];

void emit_help(void);
void parse_version(uint16_t reqid, unsigned char const *const response,
                   ssize_t len);
void skip_name(unsigned char const **const rp, unsigned char const *const end);
int udp_connect(const char *host, const char *port);

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("dns inet stdio unveil", NULL) == -1) err(1, "pledge failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    int ch;
    while ((ch = getopt(argc, argv, "h?46n")) != -1) {
        switch (ch) {
        case '4': hints.ai_family = AF_INET; break;
        case '6': hints.ai_family = AF_INET6; break;
        case 'n': hints.ai_flags |= AI_NUMERICHOST | AI_NUMERICSERV; break;
        case 'h':
        case '?':
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;
    if (argc < 1 || argc > 2) emit_help();

    int sock = udp_connect(argv[0], argc == 2 ? argv[1] : "53");

    uint16_t reqid;
    arc4random_buf(&reqid, sizeof(reqid));
    memcpy(query, &reqid, sizeof(reqid));

    if (send(sock, query, REQLEN, 0) == -1) err(1, "send failed");
    ssize_t len;
    if ((len = recv(sock, response, 512, 0)) == -1) err(1, "recv failed");
    if (len < DNS_HDR_LEN) errx(1, "response is too short");

    close(sock);
#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif
    parse_version(reqid, response, len);
    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: dns-server-version [-4 | -6] [-n] host [port]\n", stderr);
    exit(EX_USAGE);
}

void parse_version(uint16_t reqid, unsigned char const *const response,
                   ssize_t len) {
    uint16_t resid;
    memcpy(&resid, response, sizeof(resid));
    if (resid != reqid) warnx("id mismatch %04X %04X", reqid, resid);

    if ((response[2] & 0x80) >> 7 != 1) warnx("query flag set in response??");

    int rcode     = response[3] & 0x07;
    int questions = (response[4] << 8) + response[5];
    int answers   = (response[6] << 8) + response[7];
    if (answers == 0) errx(1, "no answer (rcode %d)", rcode);
    if (rcode != 0) warnx("non-zero rcode %d", rcode);

    unsigned char const *rp        = response + DNS_HDR_LEN;
    unsigned char const *const end = response + len;

    for (int i = 0; i < questions; i++) {
        skip_name(&rp, end);
        // QTYPE and QCLASS (16-bit)
        rp += 4;
    }

    // assume the relevant response is in the first answer
    if (answers > 1) warnx("more than one answer??");
    skip_name(&rp, end);
    // TYPE and CLASS (16-bit), TTL (32-bit)
    rp += 8;
    if (rp >= end) errx(1, "parsed past end of message??");

    uint16_t rdlen;
    memcpy(&rdlen, rp, sizeof(rdlen));
    rdlen = ntohs(rdlen);
    rp += sizeof(rdlen);
    if (rp + rdlen > end) errx(1, "data extends past end of message??");

    // TODO can the version text include pointers like the labels can?
    int txtlen = *rp++;
    if (rp + txtlen > end) errx(1, "text extends past end of message??");
    printf("%.*s\n", txtlen, rp);
}

/* [RFC 1035] "4.1.4. Message compression" gives three forms 1) sequence
 * of labels ending in zero octet 2) pointer 3) sequence of labels
 * ending in a pointer. so assuming this is followed and hasn't been
 * updated since... */
inline void skip_name(unsigned char const **const rp,
                      unsigned char const *const end) {
    if (*rp + 2 >= end) errx(1, "parsed past end of message??");
    while (**rp) {
        if ((**rp & 0xC0) == 0xC0) { // 14-bit pointer (ignored)
            *rp += 2;
            return;
        } else { // 6-bit length and that amount
            // high 10 and 01 are "reserved for future use" per RFC
            if ((**rp >> 6) != 0) warnx("non-zero high bits in label length??");
            *rp += 1 + **rp & 0x3F;
        }
    }
    *rp += 1; // trailing zero octet
}

int udp_connect(const char *host, const char *port) {
    int ret;
    struct addrinfo *allpeers, *peer;
    if ((ret = getaddrinfo(host, port, &hints, &allpeers)) != 0)
        errx(1, "getaddrinfo failed: %.100s", gai_strerror(ret));
    int sock = -1;
    peer     = allpeers;
    while (peer) {
        if ((sock = socket(peer->ai_family, peer->ai_socktype,
                           peer->ai_protocol)) == -1) {
            warn("socket failed");
            goto NEXTPEER;
        }
        if (connect(sock, peer->ai_addr, peer->ai_addrlen) == -1) {
            warn("connect failed");
            close(sock);
            sock = -1;
        } else {
            break;
        }
    NEXTPEER:
        peer = peer->ai_next;
    }
    if (sock == -1) errx(1, "unable to connect");
    freeaddrinfo(allpeers);
    return sock;
}
