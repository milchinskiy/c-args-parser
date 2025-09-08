// Demonstrates: --md [FILE], --man [SEC:FILE], --completion [SHELL[:FILE]]
#include <stdio.h>
#include <string.h>

#include "c-args-parser.h"

typedef struct {
    int                     did;
    const cargs_env        *envp;
    const struct cargs_cmd *rootp;
} st;

/* --md [FILE] */
static int cb_md(const char *v, void *u) {
    st   *S = (st *)u;
    FILE *f = v && *v ? fopen(v, "w") : stdout;
    if (!f) {
        perror("fopen");
        return -1;
    }
    cargs_emit_markdown(S->envp, S->rootp, S->envp->prog ? S->envp->prog : "prog", f);
    if (f != stdout) fclose(f);
    S->did = 1;
    return CARGS_OK;
}

/* --man [SEC:FILE] or --man [FILE] (defaults to section 1) */
static int cb_man(const char *v, void *u) {
    st         *S    = (st *)u;
    const char *sec  = "1";
    const char *file = NULL;
    char        secbuf[8];

    if (v && *v) {
        const char *colon = strchr(v, ':');
        if (colon) {
            size_t n = (size_t)(colon - v);
            if (n >= sizeof(secbuf)) n = sizeof(secbuf) - 1;
            memcpy(secbuf, v, n);
            secbuf[n] = '\0';
            sec       = secbuf;
            file      = colon + 1;
        } else {
            file = v;
        }
    }

    FILE *f = file && *file ? fopen(file, "w") : stdout;
    if (!f) {
        perror("fopen");
        return -1;
    }
    cargs_emit_man(S->envp, S->rootp, S->envp->prog ? S->envp->prog : "prog", f, sec);
    if (f != stdout) fclose(f);
    S->did = 1;
    return CARGS_OK;
}

/* --completion [SHELL[:FILE]] where SHELL ∈ {bash,zsh,fish} */
static int cb_completion(const char *v, void *u) {
    st         *S     = (st *)u;
    const char *shell = "bash";
    const char *file  = NULL;
    char        shbuf[16];

    if (v && *v) {
        const char *colon = strchr(v, ':');
        if (colon) {
            size_t n = (size_t)(colon - v);
            if (n >= sizeof(shbuf)) n = sizeof(shbuf) - 1;
            memcpy(shbuf, v, n);
            shbuf[n] = '\0';
            shell    = shbuf;
            file     = colon + 1;
        } else {
            shell = v;
        }
    }

    FILE *f = file && *file ? fopen(file, "w") : stdout;
    if (!f) {
        perror("fopen");
        return -1;
    }

    const char *prog = S->envp->prog ? S->envp->prog : "prog";
    if (strcmp(shell, "bash") == 0) cargs_emit_completion_bash(S->envp, S->rootp, prog, f);
    else if (strcmp(shell, "zsh") == 0) cargs_emit_completion_zsh(S->envp, S->rootp, prog, f);
    else if (strcmp(shell, "fish") == 0) cargs_emit_completion_fish(S->envp, S->rootp, prog, f);
    else {
        fprintf(stderr, "unknown shell: %s\n", shell);
        if (f != stdout) fclose(f);
        return -1;
    }

    if (f != stdout) fclose(f);
    S->did = 1;
    return CARGS_OK;
}

/* If nothing chosen, show help */
static int run_root(int argc, char **argv, void *user) {
    st *S = (st *)user;
    (void)argc;
    (void)argv;
    if (!S->did) { cargs_print_help(S->envp, S->rootp, S->envp->prog ? S->envp->prog : "prog", NULL, 0); }
    return CARGS_OK;
}

int main(int argc, char **argv) {
    st              S      = {0};

    const cargs_opt opts[] = {
        {"md",         0, CARGS_ARG_OPTIONAL, "[FILE]",         "Emit Markdown to FILE or stdout",     cb_md,         NULL, NULL, 0,
         CARGS_GRP_NONE                                                                                                                            },
        {"man",        0, CARGS_ARG_OPTIONAL, "[SEC:FILE]",     "Emit man(7) to FILE (default sec=1)", cb_man,        NULL, NULL, 0,
         CARGS_GRP_NONE                                                                                                                            },
        {"completion", 0, CARGS_ARG_OPTIONAL, "[SHELL[:FILE]]", "Emit completion for bash/zsh/fish",   cb_completion,
         NULL,                                                                                                              NULL, 0, CARGS_GRP_NONE},
    };

    const cargs_cmd root = {
        .name        = NULL,
        .desc        = "Docs & completions example",
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
        .prog         = "ex-docs",
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

    S.envp  = &env;
    S.rootp = &root;
    return cargs_dispatch(&env, &root, argc, argv, &S) < 0 ? 1 : 0;
}
