#include "flag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct strnode strnode;
struct strnode {
    const char *str;
    struct strnode *next; 
};

void strlist_add(strnode **headp, const char *str)
{
    strnode *tmp;
    tmp = malloc(sizeof(strnode));
    tmp->next = *headp;
    tmp->str = str;
    *headp = tmp;
}

void strlist_free(strnode **headp)
{
    strnode *tmp;
    while (*headp) {
        tmp = *headp;
        *headp = (*headp)->next;
        free(tmp);
    }
}

int strlist_contain(strnode **headp, const char *str)
{
    strnode *tmp = *headp;
    while (tmp) {
        if (strcmp(tmp->str, str) == 0)
            return 1;
        tmp = tmp->next;
    }
    return 0;
}

int flag_exclude_callback(flag_config *config)
{
    strnode **stringsp = config->flag->value;
    strlist_add(stringsp, config->argument);
    return 0;
}

enum { read = 1<<0, write = 1<<1, exec = 1<<2 };

int main(int argc, const char **argv)
{
    flag_config config;
    strnode *strings = NULL;
    int help = 0;
    int perms = 0;
    int i;
    cli_flag flags[] = {
        FLAG_GROUP("Basic options"),
        FLAG_COMMON(flag_require_arg, '\0', "exclude", "<STRING>",
                    "string to exclude. can using multiple times",
                    &strings, &flag_exclude_callback, {0}),
        FLAG_BOOL('h', "help", "print this message", &help),
        FLAG_GROUP("Bit flags"),
        FLAG_BIT(0, "read", "read permition", &perms, read),
        FLAG_BIT(0, "write", "write permition", &perms, write),
        FLAG_BIT(0, "exec", "exec permition", &perms, exec),
    };
    flag_init(&config, argv[0], flags, sizeof(flags)/sizeof(cli_flag));
    argc = flag_parse(&config, argv);
    if (help) {
        flag_usage(&config, "Start of usage", "End of usage");
        return 0;
    }
    for (i = 1; i < argc; i++) {
        if (!strlist_contain(&strings, argv[i]))
            printf("%s\n", argv[i]);
    }
    if (perms)
        printf("perms: %d\n", perms);
    return 0; 
}
