#include <stdio.h>

#include "c-args-parser.h"

typedef struct {
    int dry;
    int verbose;
} st;

static int cb_dry(const char *v, void *u) {
    (void)v;
    ((st *)u)->dry = 1;
    return CARGS_OK;
}

static int cb_v(const char *v, void *u) {
    (void)v;
    ((st *)u)->verbose = 1;
    return CARGS_OK;
}

/* Schema: SRC{1..inf} DST{1} */
static const cargs_pos pos_schema[] = {
    {"SRC", NULL, 1, CARGS_POS_INF},
    {"DST", NULL, 1, 1            }
};

static int run_cp(int argc, char **argv, void *user) {
    st *S = (st *)user;
    if (argc < 2) {
        fprintf(stderr, "need SRC... DST\n");
        return 1;
    }
    int dst_i = argc - 1;
    printf("Copy to %s (%s%s)\n", argv[dst_i], S->dry ? "DRY-RUN" : "LIVE", S->verbose ? ", verbose" : "");
    for (int i = 0; i < dst_i; i++) printf("  - %s\n", argv[i]);
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st              S      = {0};
    const cargs_opt opts[] = {
        {"dry-run", 'n', CARGS_ARG_NONE, NULL, "Do not actually copy (dry run)", cb_dry, NULL, NULL, 0, CARGS_GRP_NONE},
        {"verbose", 'V', CARGS_ARG_NONE, NULL, "Verbose output",                 cb_v,   NULL, NULL, 0, CARGS_GRP_NONE},
    };
    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "cp-like",
        .opts        = opts,
        .opt_count   = sizeof(opts) / sizeof(opts[0]),
        .subs        = NULL,
        .sub_count   = 0,
        .aliases     = NULL,
        .alias_count = 0,
        .pos         = pos_schema,
        .pos_count   = 2,
        .run         = run_cp
    };

    cargs_env env = {
        .prog         = "ex-cp-like",
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
