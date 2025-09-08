#include <stdio.h>

#include "c-args-parser.h"

typedef struct {
    int json, yaml;
    int light, dark;
} st;

static int cb_json(const char *v, void *u) {
    (void)v;
    ((st *)u)->json = 1;
    return CARGS_OK;
}

static int cb_yaml(const char *v, void *u) {
    (void)v;
    ((st *)u)->yaml = 1;
    return CARGS_OK;
}

static int cb_light(const char *v, void *u) {
    (void)v;
    ((st *)u)->light = 1;
    return CARGS_OK;
}

static int cb_dark(const char *v, void *u) {
    (void)v;
    ((st *)u)->dark = 1;
    return CARGS_OK;
}

static int run_root(int argc, char **argv, void *user) {
    (void)argc;
    (void)argv;
    st *S = (st *)user;
    printf("format=%s, theme=%s\n", S->json ? "json" : "yaml", S->light ? "light" : "dark");
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st S = {0};

    /* group 1: XOR (choose at most one), group 2: REQ_ONE (exactly one) */
    const cargs_opt opts[] = {
        {"json",  0, CARGS_ARG_NONE, NULL, "Output JSON", cb_json,  NULL, NULL, 1, CARGS_GRP_XOR    },
        {"yaml",  0, CARGS_ARG_NONE, NULL, "Output YAML", cb_yaml,  NULL, NULL, 1, CARGS_GRP_XOR    },
        {"light", 0, CARGS_ARG_NONE, NULL, "Light theme", cb_light, NULL, NULL, 2, CARGS_GRP_REQ_ONE},
        {"dark",  0, CARGS_ARG_NONE, NULL, "Dark theme",  cb_dark,  NULL, NULL, 2, CARGS_GRP_REQ_ONE},
    };

    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "output/theme groups",
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
        .prog         = "ex-groups",
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
