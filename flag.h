#ifndef FLAG_H_SENTRY
#define FLAG_H_SENTRY

typedef struct flag_config_tag flag_config;

enum flag_types { flag_no_arg, flag_require_arg, flag_usage_group };

/* cli flag
 *   type:         flag type from enum flag_types
 *   short_name:   single character flag name, '\0' if none
 *   long_name:    long flag name, null if none
 *   arg_format:   format description for argument if required, null if none
 *   description:  flag description for usage output, null if none
 *   value:        pointer to variable storing flag value
 *   callback:     function called to parse argument value or implement custom
 *                 logic
 *   extra:        additional data that can be used by developer in callback
 *                 functions
 */
typedef int (flag_callback_fn)(flag_config *config);

typedef struct {
    char type;
    char short_name;
    const char *long_name;
    const char *arg_format;
    const char *description;
    void *value;
    flag_callback_fn *callback;
    union {
        int int_data;
        double double_data;
        float float_data;
        char char_data;
        const char *str_data;
        void *ptr;
    } extra;
} cli_flag;

enum { flag_max_name_length = 31 };

/* flag configuration
 *   program:    program name
 *   flag:       current flag being processed
 *   flag_name:  name of current flag (as appeared in command line)
 *   argument:   argument value for current flag
 *   flags:      array of flags
 *   flag_count: count of flags
 */
struct flag_config_tag {
    cli_flag *flag;
    char flag_name[flag_max_name_length+1];
    const char *argument;
    const char *program;
    cli_flag *flags;
    int flag_count;
};

void flag_init(flag_config *config, const char *program,
               cli_flag *flags, int flag_count);

/* parse command line arguments according to registered flags.
 * modifies argv array in-place, leaving only argv[0] and positional arguments.
 * returns:
 *   >0 : new argc value on success
 *   <0 : on parsing error
 */
int flag_parse(flag_config *config, int argc, const char **argv);

void flag_usage(flag_config *config, const char *descr, const char *epilog);

/* can be used inside a custom flag callback to output an error message */
void flag_error(const flag_config *config, const char *msg);

/* built-in flags */
flag_callback_fn flag_int_callback, flag_double_callback,
                 flag_string_callback, flag_bool_callback,
                 flag_bit_callback;

#define FLAG_COMMON(type, short_name, long_name, arg_format, \
                    description, value, callback, extra) \
    { \
        type, short_name, long_name, \
        arg_format, description, \
        value, callback, extra \
    }

#define FLAG_GROUP(group_name) \
    FLAG_COMMON(flag_usage_group, 0, NULL, \
                NULL, group_name, \
                NULL, NULL, {0})

#define FLAG_INT(short_name, long_name, arg_format, description, value) \
    FLAG_COMMON(flag_require_arg, short_name, long_name, \
                arg_format, description, \
                value, &flag_int_callback, {0})

#define FLAG_DOUBLE(short_name, long_name, arg_format, description, value) \
    FLAG_COMMON(flag_require_arg, short_name, long_name, \
                arg_format, description, \
                value, &flag_double_callback, {0})

#define FLAG_STRING(short_name, long_name, arg_format, description, strptr) \
    FLAG_COMMON(flag_require_arg, short_name, long_name, \
                arg_format, description, \
                strptr, &flag_string_callback, {0})

#define FLAG_BOOL(short_name, long_name, description, value) \
    FLAG_COMMON(flag_no_arg, short_name, long_name, \
                NULL, description, \
                value, &flag_bool_callback, {0})

#define FLAG_BIT(short_name, long_name, description, value, bit) \
    FLAG_COMMON(flag_no_arg, short_name, long_name, \
                NULL, description, \
                value, &flag_bit_callback, {bit})

#endif /* FLAG_H_SENTRY */
