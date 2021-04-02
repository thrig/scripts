// ow - shortcuts and directory to URL mapping

#include <sys/types.h>

#include <ctype.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <EXTERN.h>
#include <perl.h>

// set this large enough to avoid realloc on the typical browser
// commands exec'd out to. minimum is 3: command, url, (char *)0
#ifndef OW_ARGC
#define DEFAULT_ARGC 8
#endif

// default command to visit() URL with
#ifndef OW_COMMAND
#define DEFAULT_COMMAND "w3m"
#endif

int Flag_Alwaysremap; // -A
char *Flag_Chdir;     // -C
int Flag_Dirmap;      // -d or "wv"
int Flag_Listurl;     // -l
char *Flag_Command;   // -o
char *Flag_Tag;       // -t

char *Config_Dir; // ~/.ow by default, see setup()

static PerlInterpreter *my_perl;
static void xs_init(pTHX);
EXTERN_C void boot_DynaLoader(pTHX_ CV *cv);

void dirmap(int, char **);
void emit_usage(void);
char *fill_in(int, char **);
char *host_or_url(char *);
char *path_to(char *);
char *query(DB *, char *, size_t);
char *remap(char *);
void setup(int, char **, char **);
void shortcut(int, char **);
void shortcut_onearg(int, char **, DB *);
void shortcut_args(int, char **, DB *);
char **tokenize(char *, char *);
void visit(char *);

int main(int argc, char *argv[], char *env[]) {
    int ch;

#ifdef __OpenBSD__
    // prot_exec is due to the xs_init so can go if that goes
    if (pledge("exec prot_exec rpath stdio", NULL) == -1)
        err(1, "pledge failed");
#endif

    if (strcmp(getprogname(), "wv") == 0) Flag_Dirmap = 1;

    while ((ch = getopt(argc, argv, "h?AC:dlo:t:")) != -1) {
        switch (ch) {
        case 'A': Flag_Alwaysremap = 1; break;
        case 'C': Flag_Chdir = optarg; break;
        case 'd': Flag_Dirmap = 1; break;
        case 'l': Flag_Listurl = 1; break;
        case 'o': Flag_Command = optarg; break;
        case 't': Flag_Tag = optarg; break;
        case 'h':
        case '?':
        default: emit_usage();
        }
    }
    argc -= optind;
    argv += optind;

    setup(argc, argv, env);

    if (Flag_Dirmap) // -d or "wv"
        dirmap(argc, argv);
    else
        shortcut(argc, argv);

    // NOTE may exec some other program before here
    exit(EXIT_SUCCESS);
}

// "wv" or -d: generate some sort of URL from a file or the current
// working directory
void dirmap(int argc, char *argv[]) {
    char *path, *url = NULL;

    // use the file supplied or instead the current working directory
    if (argc > 1) emit_usage();
    if (argc == 1) {
        if (**argv == '\0') emit_usage();
        if ((path = realpath(*argv, NULL)) == NULL) err(1, "realpath failed");
    } else {
        if ((path = getcwd(NULL, 0)) == NULL) err(1, "getcwd failed");
    }

    eval_pv("sub dirmap {(my $path,my $re,$_,my $tag)=@_;"
            "if ($path =~ m/$re/) {"
            "my %tmpl = %+; $tmpl{tag} = $tag if defined $tag;"
            "s/%\\{([^}]+)\\}/$tmpl{$1}/eg;"
            "return 1}return 0}",
            TRUE);
    FILE *fh;
    char *dirmap = path_to("dirmap");
    if ((fh = fopen(dirmap, "r")) == NULL)
        err(1, "could not open '%s'", dirmap);
    char *line      = NULL;
    size_t linesize = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linesize, fh)) != -1) {
        if (*line == '#' || *line == '\0' || isspace(*line)) continue;
        char *lp, *regex, *dest, *tag = NULL;
        lp = line;
        if ((regex = strsep(&lp, " \t\v\r\n")) == NULL) continue;
        if ((dest = strsep(&lp, " \t\v\r\n")) == NULL) continue;
        if (Flag_Tag && (tag = strsep(&lp, " \t\v\r\n")) != NULL)
            if (*tag == '\0' || isspace(*tag) || strcmp(Flag_Tag, tag) != 0)
                continue;
        dSP;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        EXTEND(SP, 3);
        mPUSHs(newSVpv(path, 0));
        mPUSHs(newSVpv(regex, 0));
        mPUSHs(newSVpv(dest, 0));
        if (tag != NULL) mXPUSHs(newSVpv(tag, 0));
        PUTBACK;
        call_pv("dirmap", G_SCALAR);
        SPAGAIN;
        IV ret = POPi;
        PUTBACK;
        FREETMPS;
        LEAVE;
        if (ret == 1) {
            // NOTE risky (but avoids the cost of a strdup or savepv)
            url = SvPVX(get_sv("_", 0));
            break;
        }
    }
    if (ferror(fh)) err(1, "getline failed");
    fclose(fh);

    // fallback to file:// URL generation (this might instead throw an
    // error should a mapping be deemed mandatory)
    if (url == NULL)
        if (asprintf(&url, "file://%s", path) < 0) err(1, "asprintf failed");

    visit(url);
}

void emit_usage(void) {
    fputs("Usage: ow [-Al] [-C dir] [-o cmd] [-t tag] shortcut [arg [..]]\n"
          "       ow [-Al] [-C dir] [-o cmd] hostname-or-url\n"
          "       wv [-Al] [-C dir] [-o cmd] [-t tag] [file]\n",
          stderr);
    exit(EX_USAGE);
}

// in the template (which is in argv[0]) the %@ are replaced with
// "all+the+args", and %{1} with argv[1], %{2} with argv[2], etc. also
// %{tag} will get the -t tag argument, or the usual empty string
// treatment should the tag not be present
//
// NOTE the numbered templates used to be %1, etc, but this will run
// afoul URL that have already been escaped whereby a "%80" would be
// templated into "", or "%8A" into "A" (or maybe to something else if
// there had been 80 or more likely 8 arguments passed through)
//
//   perl -Mutf8 -MURI::Escape -E 'say uri_escape_utf8("À")'
inline char *fill_in(int argc, char *argv[]) {
    eval_pv("sub template_url {my $tag = shift;$_ = shift;"
            "s/(?:%(?<key>@)|%\\{(?<key>tag|[0-9]+)})/"
            "if ($+{key} eq '@') { join '+', @_ }"
            "elsif ($+{key} eq 'tag') { $tag }"
            "else { $_[$+{key}-1] || '' }/eg}",
            TRUE);
    dSP;
    PUSHMARK(SP);
    // tag, template, argv[1], argv[2], ..
    EXTEND(SP, argc + 1);
    if (Flag_Tag != NULL)
        mPUSHs(newSVpv(Flag_Tag, 0));
    else
        mPUSHs(newSVpvs(""));
    while (*argv) {
        mPUSHs(newSVpv(*argv, 0));
        argv++;
    }
    PUTBACK;
    call_pv("template_url", G_DISCARD);
    char *url;
    // NOTE risky (but avoids the cost of a strdup or savepv)
    url = SvPVX(get_sv("_", 0));
    return url;
}

// does the argument look like a hostname or URL? if so use that as the
// URL to visit (so one can type `ow openbsd.org` which is assumed to be
// a HTTP (or now HTTPS being the fashion) request). this obviously
// rules out using "foo.bar" as a shortcut
inline char *host_or_url(char *arg) {
    char *url = NULL;

    eval_pv("sub hosturl {my ($arg) = @_;"
            "if ($arg =~ m{^\\w+(?:\\+\\w+)?://.}) {"
            "$_=$arg;return 1"
            "} elsif ($arg =~ m{[^.][.][^.]}) {"
            "$_='https://'.$arg;"
            "return 1}return 0}",
            TRUE);
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    mXPUSHs(newSVpv(arg, 0));
    PUTBACK;
    call_pv("hosturl", G_SCALAR);
    SPAGAIN;
    IV ret = POPi;
    PUTBACK;
    FREETMPS;
    LEAVE;
    if (ret == 1) {
        // NOTE risky (but avoids the cost of a strdup or savepv)
        url = SvPVX(get_sv("_", 0));
    }

    return url;
}

inline char *path_to(char *name) {
    char *path;
    if (asprintf(&path, "%s/%s", Config_Dir, name) < 0)
        err(1, "asprintf failed");
    return path;
}

inline char *query(DB *db, char *key, size_t len) {
    DBT data;
    char *value;
    int ret = (db->get)(db, &(DBT){key, len}, &data, 0);
    switch (ret) {
    case 0:
        if ((value = strndup((const char *) data.data, data.size)) == NULL)
            err(1, "strdup failed");
        return value;
    case 1: // key not found
        return NULL;
    case -1: err(1, "db query failed");
    default: warnx("unknown return code %d", ret); abort();
    }
    return NULL;
}

// optionally set the command based on the URL, e.g. to send images to
// ftp(1) or feh(1) instead of w3m(1)
inline char *remap(char *url) {
    char *command = NULL;

    eval_pv("sub remap {my ($url,$re)=@_;"
            "$url =~ m/$re/ ? 1 : 0}",
            TRUE);
    FILE *fh;
    char *remap = path_to("remap");
    if ((fh = fopen(remap, "r")) == NULL) err(1, "could not open '%s'", remap);
    char *line      = NULL;
    size_t linesize = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linesize, fh)) != -1) {
        if (*line == '#' || *line == '\0' || isspace(*line)) continue;
        char *lp, *regex, *newcmd;
        lp = line;
        if ((regex = strsep(&lp, " \t\v\r\n")) == NULL) continue;
        if ((newcmd = strsep(&lp, " \t\v\r\n")) == NULL) continue;
        dSP;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        EXTEND(SP, 2);
        mPUSHs(newSVpv(url, 0));
        mPUSHs(newSVpv(regex, 0));
        PUTBACK;
        call_pv("remap", G_SCALAR);
        SPAGAIN;
        IV ret = POPi;
        PUTBACK;
        FREETMPS;
        LEAVE;
        if (ret == 1) {
            command = newcmd;
            break;
        }
    }
    if (ferror(fh)) err(1, "getline failed");
    fclose(fh);

    return command;
}

inline void setup(int argc, char *argv[], char *env[]) {
    char *home;
    struct passwd *pass;
    if ((home = getenv("HOME")) == NULL) {
        uid_t user;
        user = getuid();
        if ((pass = getpwuid(user)) == NULL)
            err(1, "getpwuid failed for uid %d", user);
        home = pass->pw_dir;
        // perhaps the password database is wrong, or ... ?
        if (home == NULL || *home == '\0')
            errx(1, "bad HOME directory for uid %d", user);
    }
    if (asprintf(&Config_Dir, "%s/.ow", home) < 0) err(1, "asprintf failed");

    PERL_SYS_INIT3(&argc, &argv, &env);
    if ((my_perl = perl_alloc()) == NULL) errx(1, "perl_alloc failed");
    perl_construct(my_perl);
    char *embed[] = {"", "-e", "0", NULL};
    if (perl_parse(my_perl, xs_init, 3, embed, NULL) != 0)
        errx(1, "perl_parse failed");
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
}

// ow: turn a shortcut or something into a URL
void shortcut(int argc, char *argv[]) {
    DB *db;
    char *dbfile;

    if (argc < 1) emit_usage();

    dbfile = path_to("shortcuts.db");
    if ((db = dbopen(dbfile, O_RDONLY, 0666, DB_HASH, NULL)) == NULL)
        err(1, "dbopen failed '%s'", dbfile);

    if (argc == 1)
        shortcut_onearg(argc, argv, db);
    else
        shortcut_args(argc, argv, db);
}

// shortcut - expand a shortcut without arguments, or otherwise do
// various "common sense" things based on what the caller appears to
// have supplied
inline void shortcut_onearg(int argc, char *argv[], DB *db) {
    char *url;
    // +1 as DB must match the \0 on the end
    if ((url = query(db, *argv, strlen(*argv) + 1))) {
        argv[0] = url;
        visit(fill_in(1, argv));
    } else if ((url = host_or_url(*argv))) {
        visit(url);
    } else if ((url = query(db, "*", (size_t) 2))) {
        argv[0] = url;
        visit(fill_in(1, argv));
    } else {
        errx(1, "not sure what to do with '%s'", *argv);
    }
}

// shortcut@ - template the argvs into a URL
inline void shortcut_args(int argc, char *argv[], DB *db) {
    char *key, *tmpl;
    size_t len;

    len = strlen(argv[0]);
    if (len > SIZE_MAX - 2) errc(1, EOVERFLOW, "overflow??");
    // +2 for the @ and then the '\0' that the DB needs to search on
    len += 2;
    if ((key = malloc(len)) == NULL) err(1, "malloc failed");
    if (sprintf(key, "%s@", argv[0]) < 0) err(1, "sprintf failed");

    if ((tmpl = query(db, key, len))) {
        argv[0] = tmpl;
        visit(fill_in(argc, argv));
    } else if ((tmpl = query(db, "*@", (size_t) 3))) {
        // NOTE may trample the program name; if this is a problem will
        // need to instead alloc a new **blah and copy in argv[1..
        argv--;
        argc++;
        argv[0] = tmpl;
        visit(fill_in(argc, argv));
    } else {
        errx(1, "not sure what to do with '%s'", argv[0]);
    }
}

// maybe instead use a lex scanner to parse shell words? but that's
// more work
inline char **tokenize(char *command, char *url) {
    char **cargs, *token;
    size_t argc = DEFAULT_ARGC, count = 0;

    if (argc > SIZE_MAX / sizeof(char **))
        errc(1, EOVERFLOW, "cargs calloc overflow??");
    if ((cargs = calloc(argc, sizeof(char **))) == NULL)
        err(1, "calloc failed");

    while ((token = strsep(&command, " \t\v\r\n")) != NULL) {
        cargs[count++] = token;
        if (count + 2 > argc) {
            argc <<= 1;
            if (argc > SIZE_MAX / sizeof(char **))
                errc(1, EOVERFLOW, "cargs realloc overflow");
            if ((cargs = realloc(cargs, argc * sizeof(char **))) == NULL)
                err(1, "realloc failed");
        }
    }
    cargs[count++] = url;
    cargs[count]   = (char *) NULL;

    return cargs;
}

// show the URL or pass it off to some command after possibly modifying
// various things
void visit(char *url) {
    if (Flag_Listurl) {
#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif
        puts(url);
        exit(EXIT_SUCCESS);
    }

    char *command = NULL;

    if (Flag_Alwaysremap || !Flag_Command) command = remap(url);

    if (command == NULL) {
        if (Flag_Command && *Flag_Command != '\0')
            command = Flag_Command;
        else if ((command = getenv("OW_COMMAND")))
            if (*command == '\0') command = NULL;
        if (command == NULL) command = DEFAULT_COMMAND;
    }

    // commands can be replaced by entries in this database (aliases for
    // browsers, or to set obscure debug flags for lwp-download(1) or
    // similar utilities)
    DB *db;
    char *dbfile = path_to("browsers.db");
    if ((db = dbopen(dbfile, O_RDONLY, 0666, DB_HASH, NULL)) == NULL)
        err(1, "dbopen failed '%s'", dbfile);
    char *newcmd;
    if ((newcmd = query(db, command, strlen(command) + 1))) command = newcmd;

    if (Flag_Chdir)
        if (chdir(Flag_Chdir) == -1) err(1, "chdir failed '%s'", Flag_Chdir);

#ifdef __OpenBSD__
    if (pledge("exec stdio", NULL) == -1) err(1, "pledge failed");
#endif

    // ideally would like something like Text::ParseWords qw(shellwords)
    // from the Perl code here, but for now split on spaces and pass the
    // resulting tokens to exec; this disallows `sh -c "bla ..."` as the
    // command, which some (myself included) will consider as a positive
    char **cargs = tokenize(command, url);
    execvp(*cargs, cargs);
    err(1, "execlp failed '%s'", *cargs);
}

// otherwise "Can't load module Tie::Hash::NamedCapture, dynamic loading
// not available in this perl"; need %+ support for easy url templating
// in dirmap (this is rumored to be fixed in perl 5.32?)
EXTERN_C void xs_init(pTHX) {
    char *file = __FILE__;
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

// yes there is lots of memory not being free'd nor database handles
// closed but the script exits or execs something else and the database
// handles are read-only so hopefully that's alright
