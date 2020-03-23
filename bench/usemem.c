/*
 # (Ab)uses memory via the allocation of a block of memory, with a given
 # number of worker threads that then read and write values randomly
 # within that space. This code was motivated by the fact that while
 # `perl -e '1 while 1' &` can run up the CPU, one is more likely to run
 # into fork limits than CPU bottlenecks on modern multi-processor
 # systems. This code better exercises a system. Basically, it offers a
 # means to bog a system down, presumably for testing purposes, race
 # condition exploration, or to annoy the local sysadmin.
 #
 # System resource limits may need to be adjusted; see login.conf(5) on
 # OpenBSD, for example. There may also be per-process limitations. A
 # good way to bog the system down is to run something like:
 #
 #   for i in ...; do ./usemem -M -t ... & done
 #
 # Where the number of processes to launch and the thread count are
 # suitable to the number of CPUs or desired level of insanity. A timed
 # pkill(1) or easy access to reset the test system should be arranged
 # for, given that too many instances of this program may render a
 # system quite inoperative--a desktop linux system `vmstat 1` recorded
 # 12 million context switches in an entry, presumably over some time
 # period longer than a second, given how poorly that system was
 # performing. Shortly thereafter, the display froze up, and the system
 # had to be power cycled (I could still ping the box).
 #
 # The original intent of this code was to try to trigger a race
 # condition on:
 #
 #   alarm(...);
 #   somethingthatblocksforever();
 #   alarm(0);
 #     // -- paraphrased from Stevens, APUE (1st edition), p. 286.
 #
 # whereby the system is made busy enough that the SIGALRM happens
 # before the somethingthatblocks() call can be started, but after the
 # alarm(...) is established, so that the process then blocks forever.
 # It is perhaps far more likely that the system will be rendered
 # unusable (and monitoring notice this) than to hit this unlikely edge
 # case. If a system has been inordinately busy or slow, a reboot may
 # well be in order as soon as is practical. This will eliminate the
 # possiblilty that any process remains stuck on some rare edge case
 # such as the above.
 #
 # Portability: -M does sensible things on OpenBSD; on Linux and Mac OS
 # X it runs afoul various amounts of stupidity or insanity that may
 # require `-M -m upperlimit` to set the maximum allowed allocation.
 # This will help prevent the OOM killer from blasting away or the
 # kernel optimistically trying to allocate 17,592,186,044,416 integers
 # and then somehow getting bogged down.
 #
 # The code used to try to realloc() the array upwards, if possible,
 # though that on Linux ran afoul the OOM killer. Since dead processes
 # do not much tax a system, only downsizes are done. Launch more
 # instances of this script to use up the remaining memory.
 #
 # Swapping might be induced by starting an instance, then sending it
 # the CONT signal, then starting more instances? Need to test this.
 */

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

// https://github.com/thrig/goptfoo
#include <goptfoo.h>

// https://github.com/thrig/libjkiss
#include <jkiss.h>

#define MOSTMEMPOSSIBLE ( (ULONG_MAX < SIZE_MAX) ? ULONG_MAX : SIZE_MAX )

bool Flag_Auto_Mem;             // -M
unsigned long Flag_Memory;      // -m (amount, though internally # of ints)
unsigned long Flag_Threads;     // -t

int *Memory;

pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Done_Malloc = PTHREAD_COND_INITIALIZER;
pthread_cond_t Never_Happens = PTHREAD_COND_INITIALIZER;

void emit_help(void);
void *worker(void *unused);

int main(int argc, char *argv[])
{
    int ch;
    pthread_t *tids;

#ifdef __OpenBSD__
    if (pledge("stdio rpath", NULL) == -1)
        err(1, "pledge failed");
#endif

    jkiss64_init(NULL);

    while ((ch = getopt(argc, argv, "h?Mm:t:")) != -1) {
        switch (ch) {

        case 'M':
            Flag_Auto_Mem = true;
            break;

        case 'm':
            Flag_Memory = flagtoul(ch, optarg, 1UL, MOSTMEMPOSSIBLE);
            break;

        case 't':
            Flag_Threads = flagtoul(ch, optarg, 1UL, ULONG_MAX);
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

    if ((Flag_Memory == 0 && !Flag_Auto_Mem) || Flag_Threads == 0)
        emit_help();

    /* Ahh gee em. Complicated. See, if you malloc() the most memory
     * possible, then there is no space for the threads. So must spin
     * the threads up first, and then binary search realloc() to find
     * the most memory possible. Presumably other instances of this
     * program or other processes will be run to eat up any remaining
     * memory on the system.
     */

    if ((tids = calloc(sizeof(pthread_t), Flag_Threads)) == NULL)
        err(EX_OSERR, "could not calloc() threads list");

    for (unsigned long i = 0; i < Flag_Threads; i++) {
        if (pthread_create(&tids[i], NULL, worker, NULL) != 0)
            err(EX_OSERR, "could not pthread_create() thread %lu", i);
    }

    if (Flag_Auto_Mem) {
        if (!Flag_Memory)
            Flag_Memory = MOSTMEMPOSSIBLE;

        Flag_Memory /= sizeof(int);

        while (Flag_Memory) {
            if ((Memory = malloc(sizeof(int) * Flag_Memory)) == NULL) {
                Flag_Memory >>= 1;
            } else {
                break;
            }
        }

        if (Memory) {
            fprintf(stderr, "info: auto-alloc picks %lu ints\n", Flag_Memory);
        } else {
            err(EX_OSERR, "could not auto-malloc() %lu ints", Flag_Memory);
        }
    } else {
        Flag_Memory /= sizeof(int);
        if ((Memory = malloc(sizeof(int) * Flag_Memory)) == NULL)
            err(EX_OSERR, "could not malloc() %lu ints", Flag_Memory);
    }

    memset(Memory, 1, Flag_Memory * sizeof(int));

    pthread_cond_broadcast(&Done_Malloc);

    pthread_mutex_lock(&Lock);
    pthread_cond_wait(&Never_Happens, &Lock);
    pthread_mutex_unlock(&Lock);

    /* NOTREACHED */
    exit(1);
}

void emit_help(void)
{
    fputs("Usage: usemem [-M|-m memory|-M -m mem] -t threads\n", stderr);
    exit(EX_USAGE);
}

void *worker(void *unused)
{
    unsigned long idx;

    pthread_mutex_lock(&Lock);
    pthread_cond_wait(&Done_Malloc, &Lock);
    pthread_mutex_unlock(&Lock);

    for (;;) {
        // do not (much) care about modulo bias as most memory amounts will
        // be vastly less than UINT64_MAX
        idx = jkiss64_rand() % Flag_Memory;
        Memory[idx] = (idx % 2 == 1)
            ? Memory[jkiss64_rand() % Flag_Memory]
            : rand();
    }

    /* NOTREACHED */
}
