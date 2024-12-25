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
typedef struct {
    char type;
    char short_name;
    const char *long_name;
    const char *arg_format;
    const char *description;
    void *value;
    int (*callback)(flag_config *config);
    union {
        int int_data;
        double double_data;
        float float_data;
        char char_data;
        void *ptr;
    } extra;
} cli_flag;

/* dynamic array of flags */
typedef struct {
    int capacity, length;
    cli_flag *array;
} flag_vector;

enum { flag_max_name_length = 31 };

/* flag configuration
 *   program:    program name
 *   flag:       current flag being processed
 *   flag_name:  name of current flag (as appeared in command line)
 *   argument:   argument value for current flag
 *   flags:      vector of all registered flags
 */
struct flag_config_tag {
    const char *program;
    cli_flag *flag;
    char flag_name[flag_max_name_length+1];
    const char *argument;
    flag_vector flags;
};

void flag_init(flag_config *config, const char *program, int capacity);
void flag_free(flag_config *config);

/* built-in flags */
void flag_int(flag_config *config, char short_name, const char *long_name,
              const char *arg_format, const char *description,
              int *value);
void flag_double(flag_config *config, char short_name, const char *long_name,
                 const char *arg_format, const char *description,
                 double *value);
void flag_string(flag_config *config, char short_name, const char *long_name,
                 const char *arg_format, const char *description,
                 const char **strptr);
void flag_bool(flag_config *config, char short_name, const char *long_name,
               const char *description,
               int *value);
void flag_bit(flag_config *config, char short_name, const char *long_name,
              const char *description, int *value,
              int bit);

/* add usage group header for organizing related flags */
void flag_group(flag_config *config, const char *group_name);

/* parse command line arguments according to registered flags.
 * modifies argv array in-place, leaving only argv[0] and positional arguments.
 * returns:
 *   >0 : new argc value on success
 *   <0 : on parsing error
 */
int flag_parse(flag_config *config, int argc, const char **argv);
void flag_usage(flag_config *config, const char *descr, const char *epilog);

/* add custom flag to configuration */
void flag_add(flag_config *config, cli_flag *flag);
void flag_error(const flag_config *config, const char *msg);

#endif /* FLAG_H_SENTRY */
