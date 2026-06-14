#include <getopt.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <pulse/pulseaudio.h>

#define FORMAT "%s"
#define SINK_NAME_MAX 256
// set_volume() issues at most three operations per run (one mute change, an
// implicit unmute, and one volume change); four leaves a margin.
#define COMMAND_MAX_OPS 4

static pa_mainloop *mainloop = NULL;
static pa_mainloop_api *mainloop_api = NULL;
static pa_context *context = NULL;
static int retval = EXIT_SUCCESS;

typedef struct Command {
    char *format;
    bool is_delta_volume;
    bool is_mute_off;
    bool is_mute_on;
    bool is_mute_toggle;
    bool is_snoop;
    int volume;
    pa_operation *ops[COMMAND_MAX_OPS];
    int op_count;
} Command;

static void wait_loop(pa_operation *op) {
    if (op == NULL) {
        return;
    }
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        if (pa_mainloop_iterate(mainloop, 1, &retval) < 0) {
            break;
        };
    }
    pa_operation_unref(op);
}

static int constrain_volume(int volume) {
    if (volume > 100) {
        return 100;
    }
    if (volume < 0) {
        return 0;
    }
    return volume;
}

static int normalize(pa_volume_t volume) {
    return (int) round(volume * 100.0 / PA_VOLUME_NORM);
}

static pa_volume_t denormalize(int volume) {
    return (pa_volume_t) round(volume * PA_VOLUME_NORM / 100);
}

// Record an operation so the caller can wait for it to complete (and release its reference).
static void track_op(Command *command, pa_operation *op) {
    if (op == NULL) {
        return;
    }
    if (command->op_count < (int) (sizeof(command->ops) / sizeof(command->ops[0]))) {
        command->ops[command->op_count++] = op;
    } else {
        pa_operation_unref(op);
    }
}

static void set_volume(pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol, void *userdata) {
    if (i == NULL) {
        return;
    }

    Command *command = (Command *) userdata;
    if (command->is_mute_on) {
        track_op(command, pa_context_set_sink_mute_by_index(c, i->index, 1, NULL, NULL));
    }
    if (command->is_mute_off) {
        track_op(command, pa_context_set_sink_mute_by_index(c, i->index, 0, NULL, NULL));
    }
    if (command->is_mute_toggle) {
        track_op(command, pa_context_set_sink_mute_by_index(c, i->index, !i->mute, NULL, NULL));
    }
    if (command->volume == -1 && !command->is_delta_volume) {
        return;
    }

    // Turn muting off on any volume change, unless muting was specifically turned on or toggled.
    if (!command->is_mute_on && !command->is_mute_toggle) {
        track_op(command, pa_context_set_sink_mute_by_index(c, i->index, 0, NULL, NULL));
    }

    int new_volume = command->is_delta_volume ? normalize(pa_cvolume_avg(&i->volume)) + command->volume : command->volume;
    pa_cvolume cvolume;
    pa_cvolume_set(&cvolume, i->volume.channels, denormalize(constrain_volume(new_volume)));
    track_op(command, pa_context_set_sink_volume_by_index(c, i->index, &cvolume, NULL, NULL));
}

static void get_server_info(__attribute__((unused)) pa_context *c, const pa_server_info *i, void *userdata) {
    if (i == NULL || i->default_sink_name == NULL) {
        return;
    }
    strncpy(userdata, i->default_sink_name, SINK_NAME_MAX - 1);
    ((char *) userdata)[SINK_NAME_MAX - 1] = '\0';
}

static void print_volume(__attribute__((unused)) pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol,
                         void *userdata) {
    if (i == NULL) {
        return;
    }

    Command *command = (Command *) userdata;
    char output[4] = "---";
    if (!i->mute) {
        snprintf(output, 4, "%d", normalize(pa_cvolume_avg(&(i->volume))));
    }

    printf(command->format, output);
    printf("\n");
    fflush(stdout);
}

static void handle_server_info_event(pa_context *c, const pa_server_info *i, void *userdata) {
    if (i == NULL || i->default_sink_name == NULL) {
        return;
    }
    pa_context_get_sink_info_by_name(c, i->default_sink_name, print_volume, userdata);
}


static void handle_sink_event(__attribute__((unused)) pa_context *c, pa_subscription_event_type_t t,
                              __attribute__((unused)) uint32_t idx, void *userdata) {
    // See: https://freedesktop.org/software/pulseaudio/doxygen/subscribe.html
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_CHANGE) {
        return;
    }
    pa_context_get_server_info(context, handle_server_info_event, userdata);

}

static int init_context(pa_context *c) {
    if (pa_context_connect(c, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        return 1;
    }
    pa_context_state_t state;
    int iterate_retval = 0;
    while (state = pa_context_get_state(c), true) {
        if (state == PA_CONTEXT_READY) {
            return 0;
        }
        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            return 1;
        }
        pa_mainloop_iterate(mainloop, 1, &iterate_retval);
    }
}

static int quit(int new_retval) {
    // Only set `retval` if it hasn't been changed elsewhere (such as by PulseAudio in `pa_mainloop_iterate()`).
    if (retval == EXIT_SUCCESS) {
        retval = new_retval;
    }
    if (context) {
        pa_context_unref(context);
    }
    if (mainloop_api) {
        mainloop_api->quit(mainloop_api, retval);
    }
    if (mainloop) {
        pa_signal_done();
        pa_mainloop_free(mainloop);
    }
    return retval;
}

static int usage(FILE *stream, int new_retval) {
    fprintf(stream, "pavolume [-h|-s|-f format|-m [on|off|toggle]|-v [+|-]number]\n");
    return quit(new_retval);
}

int main(int argc, char *argv[]) {
    Command command = {
            .format = FORMAT,
            .is_delta_volume = false,
            .is_mute_off = false,
            .is_mute_on = false,
            .is_mute_toggle = false,
            .is_snoop = false,
            .volume = -1,
            .op_count = 0,
    };

    int opt;
    while ((opt = getopt(argc, argv, "-hsf:m:v:")) != -1) {
        switch (opt) {
            case 'h':
                // help
                return usage(stdout, EXIT_SUCCESS);
            case 'f':
                // format
                command.format = optarg;
                break;
            case 'm':
                // muting
                command.is_mute_off = strcmp("off", optarg) == 0;
                command.is_mute_on = strcmp("on", optarg) == 0;
                command.is_mute_toggle = strcmp("toggle", optarg) == 0;
                if (!(command.is_mute_off || command.is_mute_on || command.is_mute_toggle)) {
                    return usage(stderr, EXIT_FAILURE);
                }
                break;
            case 's':
                // "snoop" or monitoring mode. Prints the volume level (a number between 0 and 100 inclusive) whenever
                // it changes.
                command.is_snoop = true;
                break;
            case 'v': {
                // volume between 0 and 100 inclusive; expressed either as a specific value or as a delta.
                char *endptr;
                command.volume = (int) strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0') {
                    // No digits were consumed, or there was trailing non-numeric input.
                    return usage(stderr, EXIT_FAILURE);
                }
                if ('-' == optarg[0] || '+' == optarg[0]) {
                    command.is_delta_volume = true;
                }
                break;
            }
            default:
                return usage(stderr, EXIT_FAILURE);
        }
    }

    mainloop = pa_mainloop_new();
    if (!mainloop) {
        fprintf(stderr, "Could not create PulseAudio main loop\n");
        return quit(EXIT_FAILURE);
    }

    mainloop_api = pa_mainloop_get_api(mainloop);
    if (pa_signal_init(mainloop_api) != 0) {
        fprintf(stderr, "Could not initialize PulseAudio UNIX signal subsystem\n");
        return quit(EXIT_FAILURE);
    }

    context = pa_context_new(mainloop_api, argv[0]);
    if (!context || init_context(context) != 0) {
        fprintf(stderr, "Could not initialize PulseAudio context\n");
        return quit(EXIT_FAILURE);
    }

    char default_sink_name[SINK_NAME_MAX] = {0};
    wait_loop(pa_context_get_server_info(context, get_server_info, default_sink_name));
    wait_loop(pa_context_get_sink_info_by_name(context, default_sink_name, set_volume, &command));
    // Wait for the volume/mute changes to be applied before reading the level back.
    for (int i = 0; i < command.op_count; i++) {
        wait_loop(command.ops[i]);
    }
    wait_loop(pa_context_get_sink_info_by_name(context, default_sink_name, print_volume, &command));

    if (command.is_snoop) {
        pa_context_set_subscribe_callback(context, handle_sink_event, &command);
        pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
        if (pa_mainloop_run(mainloop, &retval) != 0) {
            fprintf(stderr, "Could not run PulseAudio main loop\n");
            return quit(EXIT_FAILURE);
        }
    }
    return quit(retval);
}
