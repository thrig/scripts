/* localaddr - show local network addresses */

#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <err.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

char addr_str[INET6_ADDRSTRLEN];

int Flag_IPV4;    /* -4 */
int Flag_IPV6;    /* -6 */
int Flag_Nolocal; /* -L */

int Gni_Flags = NI_NUMERICHOST; /* no lookup, unless -R */

void emit_usage(void);
void print_addr(struct ifaddrs *addr, socklen_t len, char *interface);

int main(int argc, char *argv[]) {
    int ch;

#ifdef __OpenBSD__
    if (pledge("dns stdio", NULL) == -1) err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?46LR")) != -1) {
        switch (ch) {
        case '4': Flag_IPV4 = 1; break;
        case '6': Flag_IPV6 = 1; break;
        case 'L': Flag_Nolocal = 1; break;
        case 'R': Gni_Flags = 0; break;
        case 'h':
        case '?':
        default: emit_usage();
        }
    }
    argc -= optind;
    argv += optind;
    if (argc > 1) emit_usage();

    char *interface = NULL;
    if (argc == 1) { interface = *argv; }

    if (Flag_IPV6 == 0 && Flag_IPV4 == 0) {
        Flag_IPV4 = 1;
        Flag_IPV6 = 1;
    }

    unsigned int count = 0;

    struct ifaddrs *addrs;
    if (getifaddrs(&addrs) == -1) err(1, "getifaddrs failed");
    struct ifaddrs *addr = addrs;
    while (addr) {
        if (interface &&
            strncmp(interface, addr->ifa_name, strlen(addr->ifa_name)) != 0) {
            addr = addr->ifa_next;
            continue;
        }
        int family = addr->ifa_addr->sa_family;
        if (family == AF_INET && Flag_IPV4) {
            if (Flag_Nolocal) {
                uint32_t v4addr = ntohl(
                    ((struct sockaddr_in *) addr->ifa_addr)->sin_addr.s_addr);
                if (((v4addr >> 24) & 0xFF) == 127) {
                    addr = addr->ifa_next;
                    continue;
                }
                // TODO may also want to exclude 169.254.0.0/16 and
                // other such "Special-Use IPv4 Addresses"
            }
            count++;
            print_addr(addr, sizeof(struct sockaddr_in), interface);
        } else if (family == AF_INET6 && Flag_IPV6) {
            if (Flag_Nolocal) {
                struct in6_addr *v6addr =
                    &((struct sockaddr_in6 *) addr->ifa_addr)->sin6_addr;
                // ::1, fe80::/10
                if (IN6_IS_ADDR_LOOPBACK(v6addr) ||
                    IN6_IS_ADDR_LINKLOCAL(v6addr)) {
                    addr = addr->ifa_next;
                    continue;
                }
                // TODO any other special use v6 to worry about?
            }
            count++;
            print_addr(addr, sizeof(struct sockaddr_in6), interface);
        }
        addr = addr->ifa_next;
    }

    // freeifaddrs(addrs);
    exit(count > 0 ? EXIT_SUCCESS : 2);
}

void emit_usage(void) {
    fputs("Usage: localaddr [-4] [-6] [-L] [-R] [interface-name]\n", stderr);
    exit(EX_USAGE);
}

inline void print_addr(struct ifaddrs *addr, socklen_t len, char *interface) {
    int ret;
    if ((ret = getnameinfo(addr->ifa_addr, len, addr_str, sizeof(addr_str),
                           NULL, 0, Gni_Flags)) != 0)
        errx(1, "getnameinfo failed: %s", gai_strerror(ret));
    if (interface) {
        printf("%s\n", addr_str);
    } else {
        printf("%s %s\n", addr->ifa_name, addr_str);
    }
}
