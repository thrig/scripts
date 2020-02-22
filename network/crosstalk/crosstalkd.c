/* crosstalkd - a server for crosstalk communication */

#include <sys/socket.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

// libevent2
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>

#ifdef SYSTEMD
#include <systemd/sd-daemon.h>
#define SYSTEMD_UNSET_ENV 1
#endif

#define NSEC_IN_SEC 1000000000U
#define REPLY_BUF_SIZE 4096U

char buf[REPLY_BUF_SIZE];
struct timespec replydelay;

void accept_cb(struct evconnlistener *c, evutil_socket_t client,
               struct sockaddr *addr, int len, void *ptr);
void accept_error_cb(struct evconnlistener *c, void *ptr);
void eventcb(struct bufferevent *bev, short events, void *ptr);
void cross_event(struct bufferevent *ev_bev, short events, void *ptr);
void cross_talk(struct bufferevent *ev_bev, void *ptr);
void emit_help(void);

int main(int argc, char *argv[]) {
    int ch;
    char *sock_path;
    struct evconnlistener *c;
    struct event_base *ev_base;
    struct sockaddr_un addr;

#ifdef __OpenBSD__
    if (pledge("cpath inet rpath stdio wpath unix unveil", NULL) == -1)
        err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?C:")) != -1) {
        switch (ch) {
        case 'C':
            if (chdir(optarg) == -1) err(1, "chdir failed");
            break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) emit_help();

    /* this is really low on some systems; use -C dir and a relative
     * socket path if run into this limit? */
    if (strlen(argv[0]) >= sizeof(addr.sun_path))
        errx(EX_DATAERR, "socket name longer than sun_path");
    sock_path = argv[0];

#ifdef __OpenBSD__
    if (unveil(argv[0], "crw") == -1) err(1, "unveil failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    if ((ev_base = event_base_new()) == NULL) err(1, "event_base_new failed");

    for (int i = 0; i < REPLY_BUF_SIZE; i++)
        buf[i] = 97 + arc4random_uniform(26);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    unlink(sock_path);

    if ((c = evconnlistener_new_bind(
             ev_base, accept_cb, NULL,
             LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
             -1, (struct sockaddr *) &addr, sizeof(addr))) == NULL)
        err(1, "could not listen on %s", argv[0]);
    evconnlistener_set_error_cb(c, accept_error_cb);

    signal(SIGPIPE, SIG_IGN);

#ifdef __OpenBSD__
    if (pledge("inet stdio", NULL) == -1) err(1, "pledge failed");
#endif

#ifdef SYSTEMD
    sd_notify(SYSTEMD_UNSET_ENV, "READY=1\n");
#endif

    event_base_dispatch(ev_base);
    exit(42);
}

void accept_cb(struct evconnlistener *c, evutil_socket_t client,
               struct sockaddr *addr, int len, void *ptr) {
    struct event_base *ev_base = evconnlistener_get_base(c);
    struct bufferevent *ev_bev =
        bufferevent_socket_new(ev_base, client, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(ev_bev, cross_talk, NULL, cross_event, NULL);
    bufferevent_enable(ev_bev, EV_READ);
}

void accept_error_cb(struct evconnlistener *c, void *ptr) {
    struct event_base *ev_base = evconnlistener_get_base(c);
    int err                    = EVUTIL_SOCKET_ERROR(); // NOTE requires errno.h
    warnx("listener error (%d): %s", err, evutil_socket_error_to_string(err));
    event_base_loopexit(ev_base, NULL);
}

void cross_event(struct bufferevent *ev_bev, short events, void *ptr) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) bufferevent_free(ev_bev);
    if (events & BEV_EVENT_ERROR) warn("bufferevent error");
}

void cross_talk(struct bufferevent *ev_bev, void *ptr) {
    size_t len;
    static int no_echo      = 0;
    static size_t seen      = 0;
    static size_t threshold = 0;
    // struct evbuffer_iovec v;
    struct evbuffer *input  = bufferevent_get_input(ev_bev);
    struct evbuffer *output = bufferevent_get_output(ev_bev);
    uint32_t val;

    // int n = evbuffer_peek(input, -1, NULL, &v, 1);
    // for (int i = 0; i < n; i++) {
    //    fwrite(v.iov_base, 1, v.iov_len, stderr);
    //}
    // putc('\n', stderr);

    if (threshold == 0) threshold = 2 + arc4random_uniform(30);

    len = evbuffer_get_length(input);

    if ((val = arc4random()) > 4252017622) no_echo = 1 + (val & 15);
    if (no_echo == 0) {
        evbuffer_add_buffer(output, input);
    } else {
        // putc('N', stderr);
        evbuffer_drain(input, len);
        no_echo -= len;
        if (no_echo < 0) no_echo = 0;
    }
    if ((seen += len) > threshold) {
        size_t buflen, start;
        start              = arc4random_uniform(REPLY_BUF_SIZE / 2 - 1);
        buflen             = 1 + arc4random_uniform(REPLY_BUF_SIZE / 2 - 1);
        replydelay.tv_nsec = arc4random_uniform(NSEC_IN_SEC);
        nanosleep(&replydelay, NULL);
        evbuffer_add(output, buf + start, buflen);
        seen = threshold = 0;
    }
}

void emit_help(void) {
    fputs("Usage: crosstalkd [-C dir] socket-file\n", stderr);
    exit(EX_USAGE);
}
