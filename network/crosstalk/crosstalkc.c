/* crosstalkc - a client for crosstalk communication */

#include <sys/socket.h>
#include <sys/un.h>

#include <err.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#define NSEC_IN_SEC 1000000000U

sigset_t block;

void cross_talk(evutil_socket_t fd, short events, void *ptr);
void emit_help(void);
void eventcb(struct bufferevent *bev, short events, void *ptr);
void readcb(struct bufferevent *bev, void *ptr);
void *schedule_talk(void *ptr);
double talk_delay(void);

int main(int argc, char *argv[]) {
    struct bufferevent *ev_bev;
    struct event *ev_alarm;
    struct event_base *ev_base;
    struct sockaddr_un addr;

#ifdef __OpenBSD__
    if (pledge("rpath stdio unix unveil", NULL) == -1) err(1, "pledge failed");
#endif

    int ch;
    while ((ch = getopt(argc, argv, "h?C:")) != -1) {
        switch (ch) {
        case 'C':
            if (chdir(optarg) == -1) err(1, "chdir failed");
            break;
        case 'h':
        case '?':
        default: emit_help();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1) emit_help();

    /* this is really low on some systems; use -C dir and a relative
     * socket path if run into this limit? */
    if (strlen(argv[0]) >= sizeof(addr.sun_path))
        errx(EX_DATAERR, "socket name longer than sun_path");
    char *sock_path = argv[0];

#ifdef __OpenBSD__
    if (unveil(argv[0], "r") == -1) err(1, "unveil failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if ((ev_base = event_base_new()) == NULL) err(1, "event_base_new failed");

    if ((ev_bev = bufferevent_socket_new(ev_base, -1, BEV_OPT_CLOSE_ON_FREE)) ==
        NULL)
        errx(1, "bufferevent_socket_new failed");

    bufferevent_setcb(ev_bev, readcb, NULL, eventcb, NULL);

    // libevent may not be compiled with BEV_OPT_THREADSAFE support so
    // instead signals are used
    ev_alarm = evsignal_new(ev_base, SIGALRM, cross_talk, ev_bev);
    event_add(ev_alarm, NULL);

    sigemptyset(&block);
    sigaddset(&block, SIGALRM);

    if (bufferevent_socket_connect(ev_bev, (struct sockaddr *) &addr,
                                   sizeof(addr)) == -1)
        err(EX_IOERR, "bufferevent_socket_connect failed");

    bufferevent_enable(ev_bev, EV_READ);

#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif

    event_base_dispatch(ev_base);
    exit(42);
}

void cross_talk(evutil_socket_t fd, short events, void *ptr) {
    int ch;
    struct bufferevent *ev_bev = ptr;
    struct evbuffer *output    = bufferevent_get_output(ev_bev);
    ch                         = 65 + arc4random_uniform(26);
    evbuffer_add(output, &ch, 1);
}

void emit_help(void) {
    fputs("Usage: crosstalkc [-C dir] socket-path\n", stderr);
    exit(EX_USAGE);
}

void eventcb(struct bufferevent *ev_bev, short events, void *ptr) {
    if (events & BEV_EVENT_CONNECTED) {
        pthread_t tid;
        if (pthread_create(&tid, NULL, schedule_talk, NULL) != 0)
            err(EX_OSERR, "pthread_create failed");
    } else if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // may instead want a sleep/restart but that's complicated so just
        // bufferevent_free(ev_bev);
        exit(1);
    } else if (events & BEV_EVENT_ERROR) {
        warn("bufferevent error");
    }
}

void readcb(struct bufferevent *ev_bev, void *ptr) {
    char buf[2048];
    int n, total = 0;
    struct timespec delay;
    struct evbuffer *input = bufferevent_get_input(ev_bev);
    sigprocmask(SIG_BLOCK, &block, NULL);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        total += n;
        write(STDOUT_FILENO, buf, n);
    }
    if (total > 4) {
        delay.tv_sec  = 1 + arc4random_uniform(3);
        delay.tv_nsec = arc4random_uniform(NSEC_IN_SEC);
        nanosleep(&delay, NULL);
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void *schedule_talk(void *ptr) {
    double secs, nsecs;
    struct timespec delay;
    while (1) {
        raise(SIGALRM);
        nsecs         = modf(talk_delay(), &secs);
        delay.tv_sec  = secs;
        delay.tv_nsec = NSEC_IN_SEC * nsecs;
        nanosleep(&delay, NULL);
    }
    return (void *) 0;
}

inline double talk_delay(void) {
    double after;
    uint32_t delay = 100000000 + arc4random_uniform(100000000) +
                     arc4random_uniform(100000000) +
                     arc4random_uniform(100000000);
    after = delay / (double) NSEC_IN_SEC;
    if (arc4random() > 3865470565) after *= 3;
    return after;
}
