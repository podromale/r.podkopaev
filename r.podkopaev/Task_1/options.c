#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    int option;
    char *argument;
} event;

static int add_event(event **events, size_t *count, int option, const char *arg)
{
    event *next = realloc(*events, (*count + 1) * sizeof(**events));
    if (next == NULL)
        return -1;
    *events = next;
    next[*count].option = option;
    next[*count].argument = arg == NULL ? NULL : strdup(arg);
    if (arg != NULL && next[*count].argument == NULL)
        return -1;
    ++*count;
    return 0;
}

static int number(const char *text, unsigned long long *value)
{
    char *end;
    errno = 0;
    *value = strtoull(text, &end, 10);
    return text[0] != '\0' && text[0] != '-' && errno == 0 && *end == '\0';
}

static void print_limit(int resource, const char *label, unsigned long divisor)
{
    struct rlimit limit;
    if (getrlimit(resource, &limit) == -1) {
        perror("getrlimit");
        return;
    }
    if (limit.rlim_cur == RLIM_INFINITY)
        printf("%s=unlimited\n", label);
    else
        printf("%s=%llu\n", label,
               (unsigned long long)limit.rlim_cur / divisor);
}

static void set_limit(int resource, const char *label, const char *arg,
                      unsigned long multiplier)
{
    struct rlimit limit;
    unsigned long long value;
    if (!number(arg, &value) || value > (unsigned long long)RLIM_INFINITY / multiplier) {
        fprintf(stderr, "invalid value for %s: %s\n", label, arg);
        return;
    }
    if (getrlimit(resource, &limit) == -1) {
        perror("getrlimit");
        return;
    }
    rlim_t requested = (rlim_t)(value * multiplier);
    if (limit.rlim_max != RLIM_INFINITY && requested > limit.rlim_max) {
        fprintf(stderr, "%s exceeds the hard limit\n", label);
        return;
    }
    limit.rlim_cur = requested;
    if (setrlimit(resource, &limit) == -1)
        perror("setrlimit");
    else
        printf("%s_set=%llu\n", label, value);
}

static void apply(const event *item)
{
    char *path;
    switch (item->option) {
    case 'i':
        printf("uid=%lu euid=%lu gid=%lu egid=%lu\n", (unsigned long)getuid(),
               (unsigned long)geteuid(), (unsigned long)getgid(), (unsigned long)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1) perror("setpgid");
        break;
    case 'p':
        printf("pid=%lu ppid=%lu pgid=%lu\n", (unsigned long)getpid(),
               (unsigned long)getppid(), (unsigned long)getpgrp());
        break;
    case 'u': print_limit(RLIMIT_FSIZE, "ulimit_512byte_blocks", 512); break;
    case 'U': set_limit(RLIMIT_FSIZE, "ulimit_512byte_blocks", item->argument, 512); break;
    case 'c': print_limit(RLIMIT_CORE, "core_size_bytes", 1); break;
    case 'C': set_limit(RLIMIT_CORE, "core_size_bytes", item->argument, 1); break;
    case 'd':
        path = getcwd(NULL, 0);
        if (path == NULL) perror("getcwd");
        else { puts(path); free(path); }
        break;
    case 'v':
        for (char **entry = environ; entry != NULL && *entry != NULL; ++entry)
            puts(*entry);
        break;
    case 'V':
        if (strchr(item->argument, '=') == NULL || item->argument[0] == '=')
            fprintf(stderr, "-V expects NAME=VALUE\n");
        else if (putenv(strdup(item->argument)) == -1)
            perror("putenv");
        break;
    }
}

int main(int argc, char **argv)
{
    event *events = NULL;
    size_t count = 0;
    int option, status = 0;
    opterr = 0;
    while ((option = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (option == '?') {
            if (optopt == '-') continue;
            fprintf(stderr, "invalid option: -%c\n", optopt);
            status = 2;
            goto done;
        }
        if (option == ':') {
            fprintf(stderr, "option -%c requires an argument\n", optopt);
            status = 2;
            goto done;
        }
        if (add_event(&events, &count, option, optarg) == -1) {
            perror("realloc/strdup");
            status = 1;
            goto done;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "unexpected argument: %s\n", argv[optind]);
        status = 2;
        goto done;
    }
    for (size_t i = count; i > 0; --i)
        apply(&events[i - 1]);
done:
    for (size_t i = 0; i < count; ++i)
        free(events[i].argument);
    free(events);
    return status;
}
