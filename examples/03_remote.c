#include <stdio.h>

#include "c-args-parser.h"

static int remote_add(int argc, char **argv, void *u) {
    (void)u;
    if (argc != 2) {
        fprintf(stderr, "usage: remote add NAME URL\n");
        return 1;
    }
    printf("remote add name=%s url=%s\n", argv[0], argv[1]);
    return CARGS_OK;
}

static int remote_rm(int argc, char **argv, void *u) {
    (void)u;
    if (argc != 1) {
        fprintf(stderr, "usage: remote remove NAME\n");
        return 1;
    }
    printf("remote remove name=%s\n", argv[0]);
    return CARGS_OK;
}

static int remote_ls(int argc, char **argv, void *u) {
    (void)argc;
    (void)argv;
    (void)u;
    puts("origin  git@git.co");
    return CARGS_OK;
}

static const cargs_pos pos_add[] = {
    CARGS_POS("NAME", NULL),
    CARGS_POS("URL",  NULL),
};

static const cargs_pos pos_rm[] = {
    CARGS_POS("NAME", NULL),
};

static const char *rm_aliases[] = {"rm"};

int                main(int argc, char **argv) {
    const cargs_cmd remote_subs[] = {
        {.name        = "add",
         .desc        = "Add a remote",
         .opts        = NULL,
         .opt_count   = 0,
         .subs        = NULL,
         .sub_count   = 0,
         .aliases     = NULL,
         .alias_count = 0,
         .pos         = pos_add,
         .pos_count   = 2,
         .run         = remote_add},
        {.name        = "remove",
         .desc        = "Remove a remote",
         .opts        = NULL,
         .opt_count   = 0,
         .subs        = NULL,
         .sub_count   = 0,
         .aliases     = rm_aliases,
         .alias_count = 1,
         .pos         = pos_rm,
         .pos_count   = 1,
         .run         = remote_rm },
        {.name        = "list",
         .desc        = "List remotes",
         .opts        = NULL,
         .opt_count   = 0,
         .subs        = NULL,
         .sub_count   = 0,
         .aliases     = NULL,
         .alias_count = 0,
         .pos         = NULL,
         .pos_count   = 0,
         .run         = remote_ls },
    };

    const cargs_cmd remote = {
                       .name        = "remote",
                       .desc        = "Manage remotes",
                       .opts        = NULL,
                       .opt_count   = 0,
                       .subs        = remote_subs,
                       .sub_count   = 3,
                       .aliases     = NULL,
                       .alias_count = 0,
                       .pos         = NULL,
                       .pos_count   = 0,
                       .run         = NULL
    };

    const cargs_cmd root = {
                       .name        = NULL,
                       .desc        = "git-like subcommands",
                       .opts        = NULL,
                       .opt_count   = 0,
                       .subs        = &remote,
                       .sub_count   = 1,
                       .aliases     = NULL,
                       .alias_count = 0,
                       .pos         = NULL,
                       .pos_count   = 0,
                       .run         = NULL
    };

    cargs_env env = {
                       .prog         = "ex-remote",
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

    return cargs_dispatch(&env, &root, argc, argv, NULL) < 0 ? 1 : 0;
}
