#include <stdio.h>

#include "c-args-parser.h"

typedef struct {
    const char *name;
    int         repeat;
    int         quiet;
} st;

static int cb_name(const char *v, void *u) {
    ((st *)u)->name = v ? v : "world";
    return CARGS_OK;
}
static int cb_repeat(const char *v, void *u) {
    int n = 1;
    if (!v || cargs_read_int(v, &n)) n = 1;
    ((st *)u)->repeat = n;
    return CARGS_OK;
}
static int cb_quiet(const char *v, void *u) {
    (void)v;
    ((st *)u)->quiet = 1;
    return CARGS_OK;
}

static int run_root(int argc, char **argv, void *user) {
    (void)argc;
    (void)argv;
    st *S = (st *)user;
    if (!S->quiet) {
        for (int i = 0; i < (S->repeat > 0 ? S->repeat : 1); ++i) printf("Hello, %s!\n", S->name ? S->name : "world");
    }
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st              S      = {.name = NULL, .repeat = 1, .quiet = 0};

    const cargs_opt opts[] = {
        {"name",   'n', CARGS_ARG_REQUIRED, "NAME", "Name to greet",              cb_name,   NULL, NULL, 0, CARGS_GRP_NONE},
        {"repeat", 'r', CARGS_ARG_OPTIONAL, "N",    "Repeat N times (default 1)", cb_repeat, NULL, NULL, 0,
         CARGS_GRP_NONE                                                                                                   },
        {"quiet",  'q', CARGS_ARG_NONE,     NULL,   "No output",                  cb_quiet,  NULL, NULL, 0, CARGS_GRP_NONE},
    };

    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "hello example",
        .opts        = opts,
        .opt_count   = sizeof(opts) / sizeof(opts[0]),
        .subs        = NULL,
        .sub_count   = 0,
        .aliases     = NULL,
        .alias_count = 0,
        .pos         = NULL,
        .pos_count   = 0,
        .run         = run_root
    };

    cargs_env env = {
        .prog         = "ex-hello",
        .version      = "0.1",
        .author       = "c-args-parser",
        .auto_help    = true,
        .auto_version = true,
        .auto_author  = false,
        .wrap_cols    = 90,
        .color        = true,
        .out          = stdout,
        .err          = stderr
    };

    return cargs_dispatch(&env, &root, argc, argv, &S) < 0 ? 1 : 0;
}
