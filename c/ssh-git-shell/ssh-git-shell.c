/*
 * git-shell(1) wrapper to allow git-shell access to a particular SSH
 * public key, via a "command" limitation on that public key in the
 * authorized_keys file (see sshd(8) for details):
 *
 *   command="/path/to/this/prog" ssh-rsa AAA...
 *
 * See also the book "Pro Git" chapter 4 in particular.
 *
 * A shell version would run something like:
 *
 *   #!/bin/sh
 *   #logging...
 *   #perhaps parse SSH_ORIGINAL_COMMAND to apply additional limits...
 *   exec git-shell -c "$SSH_ORIGINAL_COMMAND"
 *
 * Though that has all the gotchas of shell though without all the
 * gotchas of whatever C code I'm managed to throw together...
 *
 * NOTE totally untested under non-ASCII or otherwise exotic locales
 * whose characters might well appear in filesystem paths.
 */

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

void emit_usage(void);
void run_git_shell(int count, char *cmds[]);

int Flag_Syslog;                /* log to syslog */

char *DEFAULT_CMD[] = { "git-shell", "-c" };

char *Orig_Cmd_Str;

int main(int argc, char *argv[])
{
    int ch;
/*    char *ocsp, *cmd, *arg1; */

    while ((ch = getopt(argc, argv, "hs")) != -1) {
        switch (ch) {
        case 's':
            Flag_Syslog = 1;
            break;
        case 'h':
        default:
            emit_usage();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if ((Orig_Cmd_Str = getenv("SSH_ORIGINAL_COMMAND")) == NULL)
        errx(EX_DATAERR, "env SSH_ORIGINAL_COMMAND not set");

    if (Flag_Syslog)
        syslog(LOG_AUTH | LOG_PID, "SSH_ORIGINAL_COMMAND='%.128s'\n",
               Orig_Cmd_Str);

    /* Nascent ACL parsing; would need sanity checks, some option to
     * limit the path (and parsing of that) to a particular directory,
     * or a configuration file of allowed actions, but at that point
     * I would doubtless switch to Perl. */
/*    ocsp = strdup(Orig_Cmd_Str);
    cmd = strsep(&ocsp, " ");
    arg1 = strsep(&ocsp, " "); */

    if (argc == 0)
        run_git_shell(2, DEFAULT_CMD);
    else
        run_git_shell(argc, argv);

    /* NOTREACHED (unless something goes very awry) */
    return 0;
}

void emit_usage(void)
{
    errx(EX_USAGE, "[-s] [git-shell -c]");
}

void run_git_shell(int count, char *cmds[])
{
    // need space for both SSH_ORIGINAL_COMMAND and the null terminator
    char *git_shell_cmd[count + 2];
    int i;

    for (i = 0; i < count; i++)
        git_shell_cmd[i] = *cmds++;

    git_shell_cmd[i++] = Orig_Cmd_Str;
    git_shell_cmd[i] = '\0';    // otherwise lolfest of 'bad address' *cough*

    if (execvp(git_shell_cmd[0], git_shell_cmd) == -1)
        /* NOTREACHED (if all goes well) */
        err(EX_OSERR, "could not exec");
}
