// Demonstrates size parsing (IEC vs SI) and pretty printing.

#include <inttypes.h>
#include <stdio.h>

#include "c-args-parser.h"

typedef struct {
    int      have_limit;
    uint64_t limit_bytes;  // IEC (KiB=1024)
    uint64_t rate_bytes;   // SI  (KB=1000) — required
    int      verbose;
} st;

static int cb_verbose(const char *v, void *u) {
    (void)v;
    ((st *)u)->verbose = 1;
    return CARGS_OK;
}

static int cb_limit(const char *v, void *u) {
    st *S         = (st *)u;
    S->have_limit = 1;
    if (!v) {
        S->limit_bytes = 0;
        return CARGS_OK;
    }  // bare -l -> 0 bytes (just to show behavior)
    uint64_t b = 0;
    if (cargs_read_size_iec(v, &b) != 0) {
        fprintf(stderr, "bad --limit: %s\n", v);
        return -1;
    }
    S->limit_bytes = b;
    return CARGS_OK;
}

static int cb_rate(const char *v, void *u) {
    if (!v) return CARGS_ERR_MISSING_VAL;
    uint64_t b = 0;
    if (cargs_read_size_si(v, &b) != 0) {
        fprintf(stderr, "bad --rate: %s\n", v);
        return -1;
    }
    ((st *)u)->rate_bytes = b;
    return CARGS_OK;
}

/* FILE{1..inf} */
static const cargs_pos pos_schema[] = {
    {"FILE", 1, CARGS_POS_INF}
};

static int run_root(int argc, char **argv, void *user) {
    st  *S = (st *)user;
    char buf_si[32], buf_iec[32];

    printf(
        "rate : %" PRIu64 " B  (%s / %s)\n", S->rate_bytes,
        cargs_fmt_bytes(S->rate_bytes, buf_si, sizeof(buf_si), false, 2),
        cargs_fmt_bytes(S->rate_bytes, buf_iec, sizeof(buf_iec), true, 2)
    );

    if (S->have_limit) {
        printf(
            "limit: %" PRIu64 " B  (%s / %s)\n", S->limit_bytes,
            cargs_fmt_bytes(S->limit_bytes, buf_si, sizeof(buf_si), false, 2),
            cargs_fmt_bytes(S->limit_bytes, buf_iec, sizeof(buf_iec), true, 2)
        );
    } else {
        puts("limit: (none)");
    }

    printf("files (%d):\n", argc);
    for (int i = 0; i < argc; i++) printf("  - %s\n", argv[i]);

    if (S->verbose) puts("[verbose] parsing complete.");
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st              S      = {0};

    const cargs_opt opts[] = {
        {"verbose", 'V', CARGS_ARG_NONE,     NULL,   "Verbose output",                         cb_verbose, NULL,    NULL,  0, CARGS_GRP_NONE},
        {"limit",   'l', CARGS_ARG_OPTIONAL, "SIZE", "Max size (IEC: KiB/MiB/...), env LIMIT", cb_limit,   "LIMIT",
         "128MiB",                                                                                                         0, CARGS_GRP_NONE},
        {"rate",    'r', CARGS_ARG_REQUIRED, "SIZE", "Throughput (SI: KB/MB/...),  env RATE",  cb_rate,    "RATE",  "5MB", 0,
         CARGS_GRP_NONE                                                                                                                     },
    };

    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "Size parsing example (IEC vs SI)",
        .opts        = opts,
        .opt_count   = sizeof(opts) / sizeof(opts[0]),
        .subs        = NULL,
        .sub_count   = 0,
        .aliases     = NULL,
        .alias_count = 0,
        .pos         = pos_schema,
        .pos_count   = 1,
        .run         = run_root
    };

    cargs_env env = {
        .prog         = "ex-sizes",
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
