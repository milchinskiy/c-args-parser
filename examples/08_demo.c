#include <stdio.h>
#include <string.h>
#include "c-args-parser.h"

/* ---- Global options (to make [options] appear) ---- */
static const cargs_opt GLOBAL_OPTS[] = {
    { .short_name='q', .long_name="quiet",  .arg=CARGS_ARG_NONE,
      .help="Reduce output noise" },
    { .short_name='C', .long_name="config", .arg=CARGS_ARG_REQUIRED,
      .metavar="FILE", .help="Path to configuration file" },
    { .long_name="jobs", .arg=CARGS_ARG_OPTIONAL,
      .metavar="N", .help="Parallel jobs (default: auto)" },
};

/* ---- Positionals ---- */
static const cargs_pos POS_ADD[] = {
    /* NAME URL */
    { .name="NAME", .desc="Remote name", .min=1, .max=1 },
    { .name="URL",  .desc="Remote URL",  .min=1, .max=1 },
};

static const cargs_pos POS_RM[] = {
    { .name="NAME", .desc="Remote name", .min=1, .max=1 },
};

static const cargs_pos POS_ROOT[] = {
    { .name="SRC",  .desc="Source path",      .min=1, .max=1 },
    { .name="DST",  .desc="Destination path", .min=1, .max=1 },
    { .name="FILE", .desc="Extra file(s)",    .min=0, .max=CARGS_POS_INF },
};

/* ---- Commands ---- */
static const cargs_cmd REMOTE_SUBS[] = {
    { .name="add", .desc="Add a remote",
      .pos=POS_ADD, .pos_count=(int)(sizeof POS_ADD/sizeof POS_ADD[0]) },
    { .name="rm",  .desc="Remove a remote",
      .pos=POS_RM,  .pos_count=(int)(sizeof POS_RM /sizeof POS_RM [0]) },
};

static const cargs_cmd REMOTE = {
    .name="remote", .desc="Manage remotes",
    .subs=REMOTE_SUBS, .sub_count=(int)(sizeof REMOTE_SUBS/sizeof REMOTE_SUBS[0]),
};

/* Top-level command */
static const cargs_cmd ROOT_SUBS[] = { REMOTE };

static const cargs_cmd ROOT = {
    .name="demo",
    .desc="Demonstration CLI using c-args-parser",
    .opts=GLOBAL_OPTS, .opt_count=(int)(sizeof GLOBAL_OPTS/sizeof GLOBAL_OPTS[0]),
    .subs=ROOT_SUBS,   .sub_count=(int)(sizeof ROOT_SUBS  /sizeof ROOT_SUBS[0]),
    .pos=POS_ROOT,     .pos_count=(int)(sizeof POS_ROOT   /sizeof POS_ROOT[0]),
};

/* ---- Tiny navigator so `demo remote --help` works ---- */
static const cargs_cmd* find_sub(const cargs_cmd* parent, const char* name) {
    if (!parent || !name) return NULL;
    for (size_t i = 0; i < parent->sub_count; ++i) {
        const cargs_cmd* c = &parent->subs[i];
        if (c->name && strcmp(c->name, name) == 0) return c;
        for (size_t a = 0; a < c->alias_count; ++a)
            if (c->aliases[a] && strcmp(c->aliases[a], name) == 0) return c;
    }
    return NULL;
}

int main(int argc, char** argv) {
    cargs_env env = {
        .color = true,
        .auto_help = true,
    };

    const char* path[16] = {0};
    size_t depth = 0;

    const cargs_cmd* cur = &ROOT;

    /* Walk subcommands that appear before --help/-h */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) break;
        const cargs_cmd* next = find_sub(cur, argv[i]);
        if (!next) break;
        path[depth++] = next->name;
        cur = next;
    }

    /* If user asked for help anywhere, print the library's pretty help */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            /* Top-level prog name is argv[0] without the path */
            const char* prog = (argv[0] && strrchr(argv[0], '/'))
                               ? strrchr(argv[0], '/') + 1 : argv[0];
            cargs_print_help(&env, cur, prog, path, depth);
            return 0;
        }
    }

    /* No --help passed: show brief usage to hint the feature */
    const char* prog = (argv[0] && strrchr(argv[0], '/')) ? strrchr(argv[0], '/') + 1 : argv[0];
    cargs_print_usage(&env, prog, path, depth, cur);
    fputs("  (run with --help for full help)\n", stdout);
    return 0;
}
