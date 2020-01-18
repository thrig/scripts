/* elideall - consumes input. similar to
 *
 *   perl -MTerm::ReadKey=ReadMode -e 'ReadMode 5;1 while readline *STDIN'
 *
 * but perhaps different from merely disabling ECHO as may be tested by
 * observing the length of SSH packets under various conditions */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#ifndef EA_BUFSIZE
#define EA_BUFSIZE 4096
#endif
char input[EA_BUFSIZE];

struct termios Original_Termios;

void reset_term(int sig);

int main(void)
{
    struct termios terminfo;
    tcgetattr(STDIN_FILENO, &terminfo);
    memcpy(&Original_Termios, &terminfo, sizeof(struct termios));
    cfmakeraw(&terminfo);
    signal(SIGINT, reset_term);
    signal(SIGQUIT, reset_term);
    signal(SIGTERM, reset_term);
    fprintf(stderr, "%d\n", getpid());
    tcsetattr(STDIN_FILENO, TCSANOW, &terminfo);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    while (1) {
        read(STDIN_FILENO, &input, EA_BUFSIZE);
    }
    exit(EXIT_SUCCESS);
}

void reset_term(int sig)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &Original_Termios);
    exit(EXIT_SUCCESS);
}
