// Demonstrates environment defaults + CLI override precedence.
// Env first (JOBS, OUT, FMT, LEVEL), CLI can override.

#include <stdio.h>
#include <string.h>

#include "c-args-parser.h"

typedef enum { FMT_JSON = 0, FMT_YAML = 1 } fmt_t;

typedef struct {
    int         jobs;   // required
    const char *out;    // required
    int         level;  // optional (bare -L -> level=1)
    fmt_t       fmt;    // required (json|yaml)
} st;

static int cb_jobs(const char *v, void *u) {
    if (!v) return CARGS_ERR_MISSING_VAL;
    int n = 0;
    if (cargs_read_int(v, &n) != 0) {
        fprintf(stderr, "bad --jobs: %s\n", v);
        return -1;
    }
    ((st *)u)->jobs = n;
    return CARGS_OK;
}

static int cb_out(const char *v, void *u) {
    if (!v || !*v) return CARGS_ERR_MISSING_VAL;
    ((st *)u)->out = v;
    return CARGS_OK;
}

static int cb_level(const char *v, void *u) {
    if (!v) {
        ((st *)u)->level = 1;
        return CARGS_OK;
    }  // bare -L
    int n = 0;
    if (cargs_read_int(v, &n) != 0) {
        fprintf(stderr, "bad --level: %s\n", v);
        return -1;
    }
    ((st *)u)->level = n;
    return CARGS_OK;
}

static int cb_format(const char *v, void *u) {
    if (!v) return CARGS_ERR_MISSING_VAL;
    st *S = (st *)u;
    if (strcmp(v, "json") == 0) S->fmt = FMT_JSON;
    else if (strcmp(v, "yaml") == 0) S->fmt = FMT_YAML;
    else {
        fprintf(stderr, "--format expects 'json' or 'yaml', got: %s\n", v);
        return -1;
    }
    return CARGS_OK;
}

static int run_root(int argc, char **argv, void *user) {
    (void)argc;
    (void)argv;
    st *S = (st *)user;
    printf("config:\n");
    printf("  jobs   : %d\n", S->jobs);
    printf("  output : %s\n", S->out);
    printf("  level  : %d\n", S->level);
    printf("  format : %s\n", S->fmt == FMT_JSON ? "json" : "yaml");
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st              S      = {.jobs = 0, .out = NULL, .level = 0, .fmt = FMT_JSON};

    const cargs_opt opts[] = {
        // REQUIRED with env/defaults:
        {"jobs",   'j', CARGS_ARG_REQUIRED, "N",    "Worker count (env JOBS, default 4)",        cb_jobs,   "JOBS",  "4",       0,
         CARGS_GRP_NONE                                                                                                                          },
        {"output", 'o', CARGS_ARG_REQUIRED, "FILE", "Output file (env OUT, default out.bin)",    cb_out,    "OUT",   "out.bin",
         0,                                                                                                                        CARGS_GRP_NONE},
        {"format", 0,   CARGS_ARG_REQUIRED, "KIND", "Format: json|yaml (env FMT, default json)", cb_format, "FMT",   "json",
         0,                                                                                                                        CARGS_GRP_NONE},

        // OPTIONAL with env/default (no default given here; LEVEL=... triggers it)
        {"level",  'L', CARGS_ARG_OPTIONAL, "N",    "Verbosity level (bare -L -> 1; env LEVEL)", cb_level,  "LEVEL", NULL,      0,
         CARGS_GRP_NONE                                                                                                                          },
    };

    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "Env defaults & override example",
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
        .prog         = "ex-env",
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
