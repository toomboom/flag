#include "flag.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define STRLEN(str) (sizeof(str)-1)

static void vector_init(flag_vector *vec, int capacity)
{
    vec->array = malloc(capacity * sizeof(cli_flag));
    vec->capacity = capacity;
    vec->length = 0;
}

static void vector_free(flag_vector *vec)
{
    free(vec->array);
}

static void vector_add(flag_vector *vec, cli_flag *flag)
{
    if (vec->length == vec->capacity) {
        vec->capacity *= 2;
        vec->array = realloc(vec->array, vec->capacity * sizeof(cli_flag));
    }
    memcpy(&vec->array[vec->length], flag, sizeof(cli_flag));
    vec->length++;
}

void flag_init(flag_config *config, const char *program, int capacity)
{
    config->program = program;
    vector_init(&config->flags, capacity);
}

void flag_free(flag_config *config)
{
    vector_free(&config->flags);
}

void flag_add(flag_config *config, cli_flag *flag)
{
    vector_add(&config->flags, flag);
}

void flag_error(const flag_config *config, const char *msg)
{
    fprintf(stderr, "%s: option %s: %s\n",
            config->program, config->flag_name, msg);
}

static int flag_int_callback(flag_config *config)
{
    char *endptr;
    long num; 
    errno = 0;
    num = strtol(config->argument, &endptr, 10);
    if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
        flag_error(config, "argument too large");
        return 1;
    }
    else if (*endptr != '\0') {
        flag_error(config, "expected decimal integer value");
        return 2;
    }
    *(int *)config->flag->value = num;
    return 0;
} 

static int flag_double_callback(flag_config *config)
{
    char *endptr;
    double num; 
    errno = 0;
    num = strtod(config->argument, &endptr);
    if (errno == ERANGE) {
        flag_error(config, "argument too large");
        return 1;
    }
    else if (*endptr != '\0') {
        flag_error(config, "invalid floating point value");
        return 2;
    }
    *(double *)config->flag->value = num;
    return 0;
} 

static int flag_string_callback(flag_config *config)
{
    
    *(const char **)config->flag->value = config->argument;
    return 0;
} 

static int flag_bool_callback(flag_config *config)
{
    
    *(int *)config->flag->value = 1;
    return 0;
} 

static int flag_bit_callback(flag_config *config)
{
    
    *(int *)config->flag->value |= config->flag->extra.int_data;
    return 0;
} 

void flag_group(flag_config *config, const char *group_name)
{
    cli_flag group = {
        flag_usage_group,
        0, NULL,
        NULL, group_name,
        NULL, NULL
    };
    flag_add(config, &group);
}

void flag_int(flag_config *config, char short_name, const char *long_name,
              const char *arg_format, const char *description,
              int *value)
{
    cli_flag flag = {
        flag_require_arg,
        short_name, long_name,
        arg_format, description,
        value, &flag_int_callback
    };
    flag_add(config, &flag);
}

void flag_double(flag_config *config, char short_name, const char *long_name,
                 const char *arg_format, const char *description,
                 double *value)
{
    cli_flag flag = {
        flag_require_arg,
        short_name, long_name,
        arg_format, description,
        value, &flag_double_callback
    };
    flag_add(config, &flag);
}

void flag_string(flag_config *config, char short_name, const char *long_name,
                 const char *arg_format, const char *description,
                 const char **strptr)
{
    cli_flag flag = {
        flag_require_arg,
        short_name, long_name,
        arg_format, description,
        strptr, &flag_string_callback
    };
    flag_add(config, &flag);
}

void flag_bool(flag_config *config, char short_name, const char *long_name,
               const char *description,
               int *value)
{
    cli_flag flag = {
        flag_no_arg,
        short_name, long_name,
        NULL, description,
        value, &flag_bool_callback
    };
    flag_add(config, &flag);
}

void flag_bit(flag_config *config, char short_name, const char *long_name,
              const char *description, int *value,
              int bit)
{
    cli_flag flag = {
        flag_no_arg,
        short_name, long_name,
        NULL, description,
        value, &flag_bit_callback,
        .extra.int_data = bit
    };
    flag_add(config, &flag);
}

typedef struct {
    int short_pos;
    const char **argv;
    const char **pos_args;
} parsing_state;

enum parsing_errors {
    unknown_flag        = -1,
    redundant_argument  = -2,
    missing_argument    = -3,
    invalid_flag        = -4,
    callback_error      = -5
};

static int handle_error(const char *program, enum parsing_errors error,
                        const char *flag_name)
{
    switch (error) {
        case unknown_flag:
            fprintf(stderr, "%s: unknown option %s\n",
                    program, flag_name);
            break;
        case redundant_argument:
            fprintf(stderr, "%s: option %s doesn't allow an argument\n",
                    program, flag_name);
            break;
        case missing_argument:
            fprintf(stderr, "%s: option %s requires an argument\n",
                    program, flag_name);
            break;
        case invalid_flag:
            fprintf(stderr, "%s: invalid option %s\n",
                    program, flag_name);
            break;
        case callback_error:
            break;
    }
    return error;
}

static cli_flag* find_short_flag(flag_config *config, char short_name)
{
    int i;
    for (i = 0; i < config->flags.length; i++) {
        cli_flag *flag = &config->flags.array[i];
        if (flag->short_name == short_name)
            return flag;
    }
    return NULL;
}

static cli_flag* find_long_flag(flag_config *config, const char *long_name)
{
    int i;
    for (i = 0; i < config->flags.length; i++) {
        cli_flag *flag = &config->flags.array[i];
        if (flag->long_name && strcmp(flag->long_name, long_name) == 0)
            return flag;
    }
    return NULL;
}

static int parse_long_flag(parsing_state *state, flag_config *config)
{
    char *eq;
    int len;
    eq = strchr(*state->argv + 2, '=');
    len = eq
        ? MIN(eq - *state->argv, flag_max_name_length)
        : flag_max_name_length;

    strncpy(config->flag_name, *state->argv, len);
    config->flag_name[len] = '\0';
    config->flag = find_long_flag(config, config->flag_name+2);
    if (!config->flag)
        return handle_error(config->program, unknown_flag,
                            config->flag_name);
    if (eq) {
        if (config->flag->type == flag_no_arg)
            return handle_error(config->program, redundant_argument, 
                                config->flag_name);
        config->argument = eq+1;
        state->argv++;
    }
    else if (config->flag->type == flag_require_arg) {
        config->argument = state->argv[1];
        if (!config->argument)
            return handle_error(config->program, missing_argument,
                                config->flag_name);
        state->argv += 2;
    }
    else {
        config->argument = NULL;
        state->argv++;
    }
    return 0;
}

static int parse_short_flag(parsing_state *state, flag_config *config)
{
    const char *flag_pos = *state->argv + state->short_pos;
    config->flag_name[0] = '-';
    config->flag_name[1] = *flag_pos;
    config->flag_name[2] = '\0';
    config->flag = find_short_flag(config, config->flag_name[1]);
    if (!config->flag)
        return handle_error(config->program, unknown_flag,
                            config->flag_name);
    if (config->flag->type == flag_require_arg) {
        if (flag_pos[1] != '\0') {
            config->argument = flag_pos+1;
            state->argv++;
        }
        else if (state->argv[1]) {
            config->argument = state->argv[1];
            state->argv += 2;
        }
        else {
            return handle_error(config->program, missing_argument,
                                config->flag_name);
        }
        state->short_pos = 1;
    }
    else {
        config->argument = NULL;
        if (flag_pos[1] != '\0') {
            state->short_pos++;
        }
        else {
            state->argv++;
            state->short_pos = 1;
        }
    }
    return 0;
}

static int is_num(const char *str)
{
    if (*str == '-')
        str++;
    if (*str == '\0')
        return 0;

    while (*str) {
        if (!isdigit(*str))        
            return 0;
        str++;
    }
    return 1;
}

int flag_parse(flag_config *config, int argc, const char **argv)
{
    parsing_state state = {
        .short_pos = 1, .argv = argv + 1,
        .pos_args = argv + 1
    };
    int parsing_status;
    while (*state.argv) {
        if (strlen(*state.argv) < 2 || is_num(*state.argv) ||
            (*state.argv)[0] != '-') /* positional */
        {
            *state.pos_args = *state.argv;
            state.pos_args++;
            state.argv++;
            continue;
        }
        else if ((*state.argv)[1] == '-') {
            if ((*state.argv)[2] == '\0') { /* -- */
                int rest = (argv+argc-1) - (state.argv+1) + 1;
                memmove(state.pos_args, state.argv+1, sizeof(char *) * rest);
                state.pos_args += rest;
                break;
            }
            else if ((*state.argv)[2] == '=') { /* --= */
                return handle_error(config->program, invalid_flag,
                                    *state.argv);
            }
            else { /* long flag */
                parsing_status = parse_long_flag(&state, config);
            }
        }
        else { /* short flag */
            parsing_status = parse_short_flag(&state, config);
        }

        if (parsing_status != 0)
            return parsing_status;
        parsing_status = config->flag->callback(config);
        if (parsing_status != 0)
            return handle_error(NULL, callback_error, NULL);
    }
    *state.pos_args = NULL;
    return state.pos_args - argv;
}

enum { 
    usage_indent_size = 2,
    usage_max_line_width = 79,
    usage_max_descr_offset = usage_max_line_width/3
};

static int calc_description_offset(const flag_config *config)
{
    int descr_offset = -1, i;
    for (i = 0; i < config->flags.length; i++) {
        const cli_flag *flag = &config->flags.array[i];
        int offset = 0;

        offset += usage_indent_size;
        if (flag->short_name)
            offset += 2;
        if (flag->short_name && flag->long_name)
            offset += STRLEN(", ");
        if (flag->long_name)
            offset += STRLEN("--") + strlen(flag->long_name);
        if (flag->arg_format)
            offset += STRLEN(" ") + strlen(flag->arg_format);
        offset += usage_indent_size;

        if (offset <= usage_max_descr_offset && offset > descr_offset)
            descr_offset = offset;
    }
    return descr_offset == -1 ? usage_max_descr_offset : descr_offset;
}

static void pretty_print(const char *str, int text_offset)
{
    int pos = text_offset, len;
    const char *word = NULL;
    for (; *str; str++) {
        if (!isspace(*str)) {
            if (!word) {
                word = str;
                len = 1;
            }
            else {
                len++;
            }
            continue;
        }
        else if (word) {
            if (pos + len > usage_max_line_width) {
                putchar('\n');
                pos = printf("%*s%.*s", text_offset, "", len, word);
            }
            else {
                pos += printf("%.*s", len, word);
            }
            word = NULL;
        }

        if (*str == '\n') {
            putchar('\n');
            pos = printf("%*s", text_offset, "");
        }
        else {
            putchar(*str);
            pos++;
        }
    }
    if (word)
        fputs(word, stdout);
}

void flag_usage(flag_config *config, const char *descr, const char *epilog)
{
    int i, pos, descr_offset;
    descr_offset = calc_description_offset(config);
    if (descr)
        puts(descr);
    for (i = 0; i < config->flags.length; i++) {
        const cli_flag *flag = &config->flags.array[i];
        if (flag->type == flag_usage_group) {
            printf("\n%s\n", flag->description);
            continue;
        }

        pos = 0;
        pos += printf("%*s", usage_indent_size, "");
        if (flag->short_name)
            pos += printf("-%c", flag->short_name);
        if (flag->short_name && flag->long_name)
            pos += printf(", ");
        if (flag->long_name)
            pos += printf("--%s", flag->long_name);
        if (flag->arg_format)
            pos += printf(" %s", flag->arg_format);

        if (flag->description) {
            if (pos <= descr_offset)
                printf("%*s", descr_offset - pos, "");
            else
                printf("\n%*s", descr_offset, "");

            pretty_print(flag->description, descr_offset);
        }
        putchar('\n');
    }
    if (epilog)
        printf("\n%s\n", epilog);
}