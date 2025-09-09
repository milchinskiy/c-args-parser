/* SPDX-License-Identifier: MIT */

#ifndef CARGS_H
#define CARGS_H

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Public API ===== */
typedef enum {
    CARGS_ARG_NONE = 0,
    CARGS_ARG_REQUIRED,
    CARGS_ARG_OPTIONAL
} cargs_arg_kind;

/* Mutually exclusive group policies */
#define CARGS_GRP_NONE    0
#define CARGS_GRP_XOR     1 /* at most one */
#define CARGS_GRP_REQ_ONE 2 /* exactly one (env/default counts) */

/* Return codes */
#define CARGS_OK              0
#define CARGS_DONE            1 /* help/version/author printed, nothing else to do */
#define CARGS_ERR_UNKNOWN     -1
#define CARGS_ERR_MISSING_VAL -2
#define CARGS_ERR_BAD_FORMAT  -3
#define CARGS_ERR_GROUP       -4
#define CARGS_ERR_POSITIONAL  -5
#define CARGS_ERR_TOO_MANY    -6

/* Option callback: value may be NULL for NONE or missing OPTIONAL. */
typedef int (*cargs_cb)(const char *value, void *user);

/* One option descriptor */
typedef struct {
    const char    *long_name;  /* e.g., "output" (for --output); NULL if none */
    char           short_name; /* e.g., 'o' (for -o); 0 if none */
    cargs_arg_kind arg;        /* NONE / REQUIRED / OPTIONAL */
    const char    *metavar;    /* e.g., "FILE" for help */
    const char    *help;       /* one-line help */
    cargs_cb       cb;         /* invoked when option is parsed */
    /* Defaults */
    const char *env; /* environment variable to read default from (applied
                        before argv) */
    const char *def; /* string default if env is unset; may be NULL */
    /* Mutually-exclusive grouping */
    uint8_t group;        /* 0 = none; 1..32 = group id */
    uint8_t group_policy; /* CARGS_GRP_* policy (recommend same for all options
                             of a group) */
} cargs_opt;

/* Positional argument schema macros */
// WARN: do not use this in function scope as it resolve in untyped struct
#define CARGS_POS(name, help)           {(name), (help), 1, 1}
#define CARGS_POS_OPT(name, help)       {(name), (help), 0, 1}
#define CARGS_POS_N(name, help, mi, ma) {(name), (help), (mi), (ma)}

/* Positional argument item schema
 * Each item consumes between min..max occurrences before the next item.
 * Use CARGS_POS_INF for unbounded max. Example schemas:
 *   { {"NAME",1,1}, {"URL",1,1} }        -> exactly two args
 *   { {"FILE",1,CARGS_POS_INF} }          -> 1 or more files
 *   { {"SRC",1,1}, {"DST",1,1} }         -> two args
 *   { {"PATTERN",0,1}, {"FILES",1,CARGS_POS_INF} } -> optional pattern then 1+
 * files
 */
#define CARGS_POS_INF 65535
typedef struct {
    const char *name; /* label for help (e.g., "FILE") */
    const char *desc; /* one-line help */
    uint16_t    min;  /* minimum occurrences for this position */
    uint16_t    max;  /* maximum occurrences (CARGS_POS_INF for unbounded) */
} cargs_pos;

/* Global configuration and I/O */
typedef struct {
    const char *prog;         /* program name */
    const char *version;      /* optional */
    const char *author;       /* optional */
    bool        auto_help;    /* provide -h/--help at every level */
    bool        auto_version; /* provide -v/--version at root only */
    bool        auto_author;  /* provide --author at root only */
    /* Pretty printing */
    int wrap_cols; /* 0 = no wrapping; >0 wrap help at this width; if <=0 and
                      env COLUMNS set, it's used */
    bool  color;   /* enable ANSI colors unless NO_COLOR is set */
    FILE *out;     /* default stdout */
    FILE *err;     /* default stderr */
} cargs_env;

/* Subcommand node */
typedef struct cargs_cmd cargs_cmd;
struct cargs_cmd {
    const char        *name;        /* subcommand name (NULL/"" for root) */
    const char        *desc;        /* brief description for help */
    const cargs_opt   *opts;        /* options for this level */
    size_t             opt_count;
    const cargs_cmd   *subs;        /* child subcommands */
    size_t             sub_count;
    const char *const *aliases;     /* array of alias strings (may be NULL) */
    size_t             alias_count; /* length of aliases array */
    /* Positional schema for this level */
    const cargs_pos *pos;       /* may be NULL */
    size_t           pos_count; /* number of items in pos */
    /*
     * run(argc, argv, user): called for the deepest matched command
     * with remaining positional arguments. Can be NULL — we'll print help.
     */
    int (*run)(int argc, char **argv, void *user);
};

/* Entry: consume argv, route to deepest subcommand, run it. */
static inline int cargs_dispatch(
    const cargs_env *env, const cargs_cmd *root, int argc, char **argv,
    void *user
);

/* Print help for a specific command (with its subcommands/options). */
static inline void cargs_print_help(
    const cargs_env *env, const cargs_cmd *cmd, const char *prog,
    const char *const *path, size_t depth
);

/* Emit full-tree docs */
static inline void cargs_emit_markdown(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
);
static inline void cargs_emit_man(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out,
    const char *section
);

/* Completion generators */
static inline void cargs_emit_completion_bash(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
);
static inline void cargs_emit_completion_zsh(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
);
static inline void cargs_emit_completion_fish(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
);

/* ===== Typed helpers (no allocation) ===== */
static inline int cargs_read_int(const char *s, int *out);
static inline int cargs_read_uint64(const char *s, uint64_t *out);
static inline int cargs_read_size(
    const char *s, uint64_t *out, bool prefer_iec
);
static inline int cargs_read_size_si(
    const char *s, uint64_t *out
); /* KB=1000 */
static inline int cargs_read_size_iec(
    const char *s, uint64_t *out
); /* KiB=1024 */
/* Format bytes into buf, returns buf; iec? uses Ki/Mi/Gi; decimals >=0 */
static inline char *cargs_fmt_bytes(
    uint64_t bytes, char *buf, size_t bufsz, bool iec, int decimals
);

/* ===== Implementation (header-only, no globals) ===== */

/* Styling */
static inline bool cargs_use_color(const cargs_env *e) {
    if (!e || !e->color) return false;
    const char *nc = getenv("NO_COLOR");
    return !(nc && *nc);
}
static inline const char *cargs_s_bold(const cargs_env *e) {
    return cargs_use_color(e) ? "\x1b[1m" : "";
} /* bold */
static inline const char *cargs_s_flag(const cargs_env *e) {
    return cargs_use_color(e) ? "\x1b[36m" : "";
} /* cyan */
static inline const char *cargs_s_cmd(const cargs_env *e) {
    return cargs_use_color(e) ? "\x1b[35m" : "";
} /* magenta */
static inline const char *cargs_s_pos(const cargs_env *e) {
    return cargs_use_color(e) ? "\x1b[33m" : "";
} /* yellow */
static inline const char *cargs_s_rst(const cargs_env *e) {
    return cargs_use_color(e) ? "\x1b[0m" : "";
} /* reset */

/* I/O helpers */
static inline FILE *cargs_out(const cargs_env *e) {
    return (e && e->out) ? e->out : stdout;
}
static inline FILE *cargs_err(const cargs_env *e) {
    return (e && e->err) ? e->err : stderr;
}

/* Columns (wrapping) */
static inline int cargs_columns(const cargs_env *e) {
    if (e && e->wrap_cols > 0) return e->wrap_cols;
    const char *col = getenv("COLUMNS");
    if (col && *col) {
        long v = strtol(col, NULL, 10);
        if (v > 0 && v < 10000) return (int)v;
    }
    return 0;
}

static inline const cargs_opt *cargs_find_long(
    const cargs_opt *opts, size_t n, const char *name
) {
    if (!opts || !name) return NULL;
    for (size_t i = 0; i < n; i++) {
        const cargs_opt *o = &opts[i];
        if (o->long_name && strcmp(o->long_name, name) == 0) return o;
    }
    return NULL;
}
static inline const cargs_opt *cargs_find_short(
    const cargs_opt *opts, size_t n, char c
) {
    if (!opts || !c) return NULL;
    for (size_t i = 0; i < n; i++)
        if (opts[i].short_name == c) return &opts[i];
    return NULL;
}

/* Value-lookahead heuristic for OPTIONAL args:
   treat next token as a value if it doesn't look like an option,
   or if it looks like a numeric literal (e.g., -12, -0.5, +3). */
static inline bool cargs_token_looks_numeric(const char *s) {
    if (!s || !*s) return false;
    if (*s == '+' || *s == '-') {
        if (isdigit((unsigned char)s[1])) return true; /* -12, +7 */
        if (s[1] == '.' && isdigit((unsigned char)s[2])) return true; /* -.5 */
        if (s[1] == '0' && (s[2] == 'x' || s[2] == 'X'))
            return true; /* -0xFF */
        return false;
    }
    return isdigit((unsigned char)s[0]);
}

/* Word-wrapping for help text */
static inline void cargs_wrap_print(
    FILE *out, const char *text, int start_col, int width
) {
    if (!text || !*text) {
        fputc('\n', out);
        return;
    }
    if (width <= 0 || start_col <= 0 || start_col + 10 > width) {
        fprintf(out, "%s\n", text);
        return;
    }
    const char *p = text;
    while (*p) {
        int         remaining = width - start_col;
        const char *line_end = p, *last_space = NULL;
        int         used = 0;
        while (line_end[0] && line_end[0] != '\n' && used < remaining) {
            if (line_end[0] == ' ') last_space = line_end;
            line_end++;
            used++;
        }
        if (!line_end[0] || line_end[0] == '\n') {
            fprintf(out, "%.*s\n", (int)(line_end - p), p);
            p = line_end + (line_end[0] == '\n' ? 1 : 0);
            if (*p) fprintf(out, "%*s", start_col, "");
            continue;
        }
        if (last_space) {
            fprintf(out, "%.*s\n%*s", (int)(last_space - p), p, start_col, "");
            p = last_space + 1;
        } else {
            fprintf(out, "%.*s\n%*s", remaining, p, start_col, "");
            p += remaining;
        }
    }
}

/* Render positional usage into buf */
static inline void cargs_render_pos_usage(
    char *buf, size_t bufsz, const cargs_cmd *cmd
) {
    if (!buf || bufsz == 0) return;
    buf[0] = '\0';
    if (!cmd || !cmd->pos || cmd->pos_count == 0) return;
    size_t n = 0;

#define APPEND(fmt, ...)                                             \
    do {                                                             \
        int _w = snprintf(                                           \
            buf + n, (n < bufsz) ? (bufsz - n) : 0, fmt, __VA_ARGS__ \
        );                                                           \
        if (_w > 0) {                                                \
            size_t _add = (size_t)_w;                                \
            n           = (n + _add < bufsz) ? (n + _add) : bufsz;   \
        }                                                            \
    } while (0)

    for (size_t i = 0; i < cmd->pos_count; i++) {
        const cargs_pos *p    = &cmd->pos[i];
        const char      *name = (p && p->name && *p->name) ? p->name : "ARG";

        if (p->max == CARGS_POS_INF) {
            /* Unbounded */
            int minreq = (p->min > 0) ? p->min : 0;
            for (int k = 0; k < minreq; ++k) APPEND(" %s", name);
            if (p->min <= 0) APPEND(" [%s ...]", name);
            else
                APPEND(
                    " [%s ...]", name
                ); /* 1..inf or more: tail optional list */
        } else if (p->max <= 1) {
            /* 0..1 or 1..1 */
            if (p->min == 0) APPEND(" [%s]", name);
            else APPEND(" %s", name);
        } else {
            /* Finite M..N with N>1 */
            int minreq = (p->min > 0) ? p->min : 0;
            for (int k = 0; k < minreq; ++k) APPEND(" %s", name);
            int opt = (int)p->max - minreq;
            for (int k = 0; k < opt; ++k) APPEND(" [%s]", name);
        }
        if (n >= bufsz) break;
    }
    if (bufsz) buf[bufsz - 1] = '\0';
#undef APPEND
}

/* Validate positionals: ensure argc lies within [sum_mins, sum_maxs] (unless
 * any INF) */
static inline int cargs_validate_positional(
    const cargs_env *env, const cargs_cmd *cmd, int argc, char **argv
) {
    (void)argv;
    if (!cmd || !cmd->pos || !cmd->pos_count) return CARGS_OK;
    unsigned sum_min = 0;
    unsigned sum_max = 0;
    bool     has_inf = false;
    for (size_t i = 0; i < cmd->pos_count; i++) {
        sum_min += cmd->pos[i].min;
        if (cmd->pos[i].max == CARGS_POS_INF) has_inf = true;
        else sum_max += cmd->pos[i].max;
    }
    if (argc < (int)sum_min) {
        fprintf(
            cargs_err(env),
            "Missing required positional(s): need at least %u, got %d\n",
            sum_min, argc
        );
        return CARGS_ERR_POSITIONAL;
    }
    if (!has_inf && argc > (int)sum_max) {
        fprintf(
            cargs_err(env),
            "Too many positionals: at most %u allowed, got %d\n", sum_max, argc
        );
        return CARGS_ERR_TOO_MANY;
    }
    return CARGS_OK;
}

static inline bool cargs__has_any_options(
    const cargs_env *env, const cargs_cmd *cmd
) {
    return (cmd && cmd->opt_count > 0) ||
           (env && (env->auto_help || env->auto_version || env->auto_author));
}

static inline void cargs_print_usage(
    const cargs_env *env, const char *prog, const char *const *path,
    size_t depth, const cargs_cmd *cmd
) {
    FILE       *out = cargs_out(env);
    const char *B = cargs_s_bold(env), *R = cargs_s_rst(env);

    char        buf[256];
    buf[0]   = '\0';
    size_t n = 0;
    if (prog) n += (size_t)snprintf(buf + n, sizeof(buf) - n, "%s", prog);
    for (size_t i = 0; i < depth; i++) {
        if (n + 1 < sizeof(buf)) buf[n++] = ' ';
        n += (size_t)snprintf(buf + n, sizeof(buf) - n, "%s", path[i]);
    }
    buf[sizeof(buf) - 1] = '\0';

    fprintf(out, "%sUsage:%s %s", B, R, buf);

    if (cargs__has_any_options(env, cmd)) fputs(" [options]", out);
    if (cmd && cmd->sub_count) fputs(" <command> [command-options]", out);

    char pbuf[192];
    cargs_render_pos_usage(pbuf, sizeof(pbuf), cmd);
    fputs(pbuf, out);

    if (!cmd || !cmd->pos || !cmd->pos_count) fputs(" [--] [args...]", out);
    fputc('\n', out);
}

static inline void cargs_print_opt_row(
    const cargs_env *env, const cargs_opt *o
) {
    FILE       *out = cargs_out(env);
    const char *F = cargs_s_flag(env), *R = cargs_s_rst(env);
    char        lhs[160];
    lhs[0]   = '\0';
    size_t n = 0;
    if (o->short_name) {
        n += (size_t)snprintf(lhs + n, sizeof(lhs) - n, "-%c", o->short_name);
        if (o->long_name) n += (size_t)snprintf(lhs + n, sizeof(lhs) - n, ", ");
    }
    if (o->long_name) {
        n += (size_t)snprintf(lhs + n, sizeof(lhs) - n, "--%s", o->long_name);
    }
    if (o->arg == CARGS_ARG_REQUIRED) {
        n += (size_t)snprintf(
            lhs + n, sizeof(lhs) - n, " %s", o->metavar ? o->metavar : "VALUE"
        );
    } else if (o->arg == CARGS_ARG_OPTIONAL) {
        n += (size_t)snprintf(
            lhs + n, sizeof(lhs) - n, "[%s]", o->metavar ? o->metavar : "VALUE"
        );
    }
    if (o->env && o->def) {
        n += (size_t)snprintf(lhs + n, sizeof(lhs) - n, " (env %s)", o->env);
    }

    char colored[200];
    if (cargs_use_color(env))
        snprintf(colored, sizeof(colored), "%s%s%s", F, lhs, R);
    else snprintf(colored, sizeof(colored), "%s", lhs);

    const int left      = 30;
    int       width     = cargs_columns(env);
    int       start_col = left + 2;
    if (width <= 0) {
        fprintf(out, "  %-30s %s\n", colored, o->help ? o->help : "");
        return;
    }
    fprintf(out, "  %-30s ", colored);
    cargs_wrap_print(out, o->help ? o->help : "", start_col + 2, width);
}

static inline void cargs_print_cmd_row(
    const cargs_env *env, const cargs_cmd *c
) {
    FILE       *out = cargs_out(env);
    const char *C = cargs_s_cmd(env), *R = cargs_s_rst(env);

    /* build command name + optional "(alias: ...)" */
    char   name[160];
    size_t n = 0;
    name[0]  = '\0';
    n += (size_t)snprintf(
        name + n, sizeof(name) - n, "%s", c->name ? c->name : ""
    );
    if (c->alias_count) {
        n += (size_t)snprintf(name + n, sizeof(name) - n, " (alias: ");
        for (size_t a = 0; a < c->alias_count; a++) {
            if (a) n += (size_t)snprintf(name + n, sizeof(name) - n, ", ");
            n += (size_t)snprintf(
                name + n, sizeof(name) - n, "%s", c->aliases[a]
            );
        }
        n += (size_t)snprintf(name + n, sizeof(name) - n, ")");
    }

    char lhs[192];
    snprintf(lhs, sizeof(lhs), "%s%s%s", C, name, R);

    const int left      = 30;
    int       width     = cargs_columns(env);
    int       start_col = left + 2;

    if (width <= 0) {
        fprintf(out, "  %-30s %s\n", lhs, c->desc ? c->desc : "");
        return;
    }
    fprintf(out, "  %-30s ", lhs);
    cargs_wrap_print(out, c->desc ? c->desc : "", start_col + 2, width);
}

static inline void cargs_print_pos_row(
    const cargs_env *env, const cargs_pos *p
) {
    FILE       *out = cargs_out(env);
    const char *P = cargs_s_pos(env), *R = cargs_s_rst(env);
    const char *name = (p && p->name && *p->name) ? p->name : "ARG";

    /* Left column (colored metavar) */
    char lhs[160];
    (void)snprintf(lhs, sizeof(lhs), "%s%s%s", P, name, R);

    /* Right column: prefer help; add (min..max) only when not 1..1 */
    char   rhs[256];
    size_t off = 0;

    if (p && p->desc && *p->desc) {
        int n = snprintf(rhs + off, sizeof(rhs) - off, "%s", p->desc);
        if (n > 0)
            off += (size_t)n < (sizeof(rhs) - off) ? (size_t)n
                                                   : (sizeof(rhs) - off - 1);
    }

    const bool need_hint = (p && (p->min != 1 || p->max != 1));
    if (need_hint) {
        if (off && off < sizeof(rhs) - 1) rhs[off++] = ' ';
        if (p->min == p->max) {
            int n = snprintf(
                rhs + off, sizeof(rhs) - off, "(x%u)", (unsigned)p->min
            );
            if (n > 0)
                off += (size_t)n < (sizeof(rhs) - off)
                           ? (size_t)n
                           : (sizeof(rhs) - off - 1);
        } else if (p->max == CARGS_POS_INF) {
            int n = snprintf(
                rhs + off, sizeof(rhs) - off, "(%u..inf)", (unsigned)p->min
            );
            if (n > 0)
                off += (size_t)n < (sizeof(rhs) - off)
                           ? (size_t)n
                           : (sizeof(rhs) - off - 1);
        } else {
            int n = snprintf(
                rhs + off, sizeof(rhs) - off, "(%u..%u)", (unsigned)p->min,
                (unsigned)p->max
            );
            if (n > 0)
                off += (size_t)n < (sizeof(rhs) - off)
                           ? (size_t)n
                           : (sizeof(rhs) - off - 1);
        }
    }

    if (off == 0) {
        /* No help provided and it's 1..1 → keep it simple */
        (void)snprintf(rhs, sizeof(rhs), "%s", "");
    }

    const int left      = 30;
    int       width     = cargs_columns(env);
    int       start_col = left + 2;

    if (width <= 0) {
        fprintf(out, "  %-30s %s\n", lhs, rhs);
        return;
    }
    fprintf(out, "  %-30s ", lhs);
    cargs_wrap_print(out, rhs, start_col + 2, width);
}

static inline void cargs_build_usage_pos(
    char *dst, size_t cap, const cargs_pos *pos, size_t npos
) {
    size_t off = 0;
#define APPEND(fmt, ...)                                                       \
    do {                                                                       \
        int _n = snprintf(                                                     \
            dst + off, (cap > off ? cap - off : 0), fmt, __VA_ARGS__           \
        );                                                                     \
        if (_n > 0)                                                            \
            off += (size_t)_n < (cap - off) ? (size_t)_n                       \
                                            : (cap - off ? cap - off - 1 : 0); \
    } while (0)

    for (size_t i = 0; i < npos; ++i) {
        const cargs_pos *p   = &pos[i];
        const char      *tok = (p->name && *p->name) ? p->name : "ARG";

        if (p->max == CARGS_POS_INF) {
            /* Variadic tail */
            if (p->min <= 0) {
                APPEND(" [%s ...]", tok);
            } else if (p->min == 1) {
                APPEND(" %s [%s ...]", tok, tok);
            } else {
                for (int k = 0; k < p->min; ++k) APPEND(" %s", tok);
                APPEND(" [%s ...]", tok);
            }
        } else if (p->max <= 1) {
            /* 0..1 or 1..1 */
            if (p->min == 0) {
                APPEND(" [%s]", tok);
            } else {
                APPEND(" %s", tok);
            }
        } else {
            /* Finite 0..N or M..N with N>1 */
            if (p->min > 0) {
                for (int k = 0; k < p->min; ++k) APPEND(" %s", tok);
            }
            int opt = (int)p->max - (p->min > 0 ? p->min : 0);
            for (int k = 0; k < opt; ++k) APPEND(" [%s]", tok);
        }
    }
#undef APPEND
}

static inline size_t cargs_join_aliases(
    const cargs_cmd *c, char *buf, size_t bufsz
) {
    size_t n = 0;
    if (!c || !buf || !bufsz) return 0;
    buf[0] = '\0';
    for (size_t i = 0; i < c->alias_count; i++) {
        const char *a = c->aliases[i];
        if (!a) continue;
        size_t need = (n ? 2 : 0) + strlen(a);
        if (n + need + 1 >= bufsz) break;
        if (n) {
            buf[n++] = ',';
            buf[n++] = ' ';
        }
        n += (size_t)snprintf(buf + n, bufsz - n, "%s", a);
    }
    return n;
}

static inline void cargs_print_help(
    const cargs_env *env, const cargs_cmd *cmd, const char *prog,
    const char *const *path, size_t depth
) {
    FILE       *out = cargs_out(env);
    const char *B = cargs_s_bold(env), *R = cargs_s_rst(env);
    cargs_print_usage(env, prog, path, depth, cmd);
    fputc('\n', out);

    /* Options header + auto options */
    bool printed_opts_header = false;
    if (env && env->auto_help) {
        if (!printed_opts_header) {
            fprintf(out, "%sOptions:%s\n", B, R);
            printed_opts_header = true;
        }
        cargs_opt a = {
            "help", 'h',  CARGS_ARG_NONE, NULL, "Show this help and exit",
            NULL,   NULL, NULL,           0,    0
        };
        cargs_print_opt_row(env, &a);
    }
    if (env && depth == 0 && env->auto_version && env->version) {
        if (!printed_opts_header) {
            fprintf(out, "%sOptions:%s\n", B, R);
            printed_opts_header = true;
        }
        cargs_opt a = {
            "version", 'v',  CARGS_ARG_NONE, NULL, "Show version and exit",
            NULL,      NULL, NULL,           0,    0
        };
        cargs_print_opt_row(env, &a);
    }
    if (env && depth == 0 && env->auto_author && env->author) {
        if (!printed_opts_header) {
            fprintf(out, "%sOptions:%s\n", B, R);
            printed_opts_header = true;
        }
        cargs_opt a = {
            "author", 0,    CARGS_ARG_NONE, NULL, "Show author and exit",
            NULL,     NULL, NULL,           0,    0
        };
        cargs_print_opt_row(env, &a);
    }

    if (cmd && cmd->opt_count) {
        if (!printed_opts_header) {
            fprintf(out, "%sOptions:%s\n", B, R);
            printed_opts_header = true;
        }
        for (size_t i = 0; i < cmd->opt_count; i++)
            cargs_print_opt_row(env, &cmd->opts[i]);
        fputc('\n', out);
    } else if (printed_opts_header) {
        fputc('\n', out);
    }

    /* Subcommands */
    if (cmd && cmd->sub_count) {
        fprintf(out, "%sCommands:%s\n", B, R);
        for (size_t i = 0; i < cmd->sub_count; i++) {
            cargs_print_cmd_row(env, &cmd->subs[i]);
        }
    }

    /* Positionals */
    if (cmd && cmd->pos_count) {
        fprintf(out, "%sPositionals:%s\n", B, R);
        for (size_t i = 0; i < cmd->pos_count; ++i)
            cargs_print_pos_row(env, &cmd->pos[i]);
    }
}

/* Apply env-var/defaults for a command level (before parsing argv at that
 * level). */
static inline void cargs_apply_env_defaults_level(
    const cargs_cmd *cmd, void *user, uint8_t *group_counts,
    size_t group_counts_len
) {
    if (!cmd || !cmd->opts) return;
    for (size_t i = 0; i < cmd->opt_count; i++) {
        const cargs_opt *o   = &cmd->opts[i];
        const char      *val = NULL;
        if (o->env) { val = getenv(o->env); }
        if (!val && o->def) { val = o->def; }
        if (val && o->cb) {
            o->cb(val, user);
            if (o->group && o->group < group_counts_len)
                group_counts[o->group]++;
        }
    }
}

static inline const cargs_cmd *cargs_find_sub(
    const cargs_cmd *cmd, const char *name
) {
    if (!cmd || !name) return NULL;
    for (size_t i = 0; i < cmd->sub_count; i++) {
        const cargs_cmd *c = &cmd->subs[i];
        if (c->name && strcmp(c->name, name) == 0) return c;
        for (size_t a = 0; a < c->alias_count; a++) {
            if (c->aliases[a] && strcmp(c->aliases[a], name) == 0) return c;
        }
    }
    return NULL;
}

/* Parse options for a given command level. Updates *idx to first non-option. */
static inline int cargs_parse_opts_level(
    const cargs_env *env, const cargs_cmd *cmd, int argc, char **argv, int *idx,
    void *user, bool allow_version, bool allow_author, const char *prog,
    const char *const *path, size_t depth
) {
    enum { MAXG = 33 }; /* groups 1..32 */
    uint8_t counts[MAXG];
    for (size_t z = 0; z < MAXG; z++) counts[z] = 0;
    uint32_t xor_mask = 0, req_mask = 0;
    if (cmd) {
        for (size_t i = 0; i < cmd->opt_count; i++) {
            const cargs_opt *o = &cmd->opts[i];
            if (o->group > 0 && o->group < MAXG) {
                if (o->group_policy == CARGS_GRP_XOR)
                    xor_mask |= (1ull << o->group);
                if (o->group_policy == CARGS_GRP_REQ_ONE)
                    req_mask |= (1ull << o->group);
            }
        }
    }

    /* Apply env/defaults first so CLI can override, and count groups */
    cargs_apply_env_defaults_level(cmd, user, counts, MAXG);

    int i = *idx;
    while (i < argc) {
        const char *arg = argv[i];
        if (!arg || arg[0] != '-') break; /* positional begins */
        if (strcmp(arg, "--") == 0) {
            i++;
            break;
        }

        if (arg[1] == '-') {
            const char *name = arg + 2;
            const char *eq   = strchr(name, '=');
            char        namebuf[96];
            const char *val = NULL;
            if (eq) {
                size_t len = (size_t)(eq - name);
                if (len >= sizeof(namebuf)) return CARGS_ERR_BAD_FORMAT;
                memcpy(namebuf, name, len);
                namebuf[len] = '\0';
                name         = namebuf;
                val          = eq + 1;
            }

            /* auto built-ins */
            if (env && env->auto_help && strcmp(name, "help") == 0) {
                cargs_print_help(env, cmd, prog, path, depth);
                *idx = argc;
                return CARGS_DONE;
            }
            if (allow_version && env && env->auto_version &&
                strcmp(name, "version") == 0 && env->version) {
                fprintf(cargs_out(env), "%s\n", env->version);
                *idx = argc;
                return CARGS_DONE;
            }
            if (allow_author && env && env->auto_author &&
                strcmp(name, "author") == 0 && env->author) {
                fprintf(cargs_out(env), "%s\n", env->author);
                *idx = argc;
                return CARGS_DONE;
            }

            const cargs_opt *o = cargs_find_long(
                cmd ? cmd->opts : NULL, cmd ? cmd->opt_count : 0, name
            );
            if (!o) {
                fprintf(cargs_err(env), "Unknown option: --%s\n", name);
                return CARGS_ERR_UNKNOWN;
            }
            if (o->arg == CARGS_ARG_REQUIRED) {
                if (!val) {
                    if (i + 1 < argc) {
                        val = argv[++i]; /* accept even if it starts with '-'
                                            (e.g., negative numbers) */
                    } else {
                        fprintf(
                            cargs_err(env), "Option '--%s' requires a value\n",
                            name
                        );
                        return CARGS_ERR_MISSING_VAL;
                    }
                }
            } else if (o->arg == CARGS_ARG_OPTIONAL) {
                if (!val && i + 1 < argc) {
                    const char *nxt = argv[i + 1];
                    if (strcmp(nxt, "--") != 0 &&
                        (nxt[0] != '-' || cargs_token_looks_numeric(nxt))) {
                        val =
                            argv[++i]; /* allow: --limit 12  and  --limit -12 */
                    }
                }
            } else {
                if (val) {
                    fprintf(
                        cargs_err(env), "Option '--%s' does not take a value\n",
                        name
                    );
                    return CARGS_ERR_BAD_FORMAT;
                }
            }
            int rc = o->cb ? o->cb(val, user) : CARGS_OK;
            if (rc < 0) return rc;
            if (o->group && o->group < MAXG) counts[o->group]++;
            i++;
            continue;
        } else {
            /* short or grouped */
            const char *p = arg + 1;
            i++;
            while (*p) {
                const char c = *p++;
                if (env && env->auto_help && c == 'h') {
                    cargs_print_help(env, cmd, prog, path, depth);
                    *idx = argc;
                    return CARGS_DONE;
                }
                if (allow_version && env && env->auto_version && c == 'v' &&
                    env->version) {
                    fprintf(cargs_out(env), "%s\n", env->version);
                    *idx = argc;
                    return CARGS_DONE;
                }
                const cargs_opt *o = cargs_find_short(
                    cmd ? cmd->opts : NULL, cmd ? cmd->opt_count : 0, c
                );
                if (!o) {
                    fprintf(cargs_err(env), "Unknown option: -%c\n", c);
                    return CARGS_ERR_UNKNOWN;
                }
                const char *val = NULL;
                if (o->arg == CARGS_ARG_REQUIRED) {
                    if (*p) {
                        val = p;
                        p += strlen(p);
                    } /* attached: -j10 */
                    else if (i < argc) {
                        val = argv[i++];
                    } /* separated: -j 10  (even if starts with '-') */
                    else {
                        fprintf(
                            cargs_err(env), "Option '-%c' requires a value\n", c
                        );
                        return CARGS_ERR_MISSING_VAL;
                    }
                } else if (o->arg == CARGS_ARG_OPTIONAL) {
                    if (*p) { /* attached: -l12 */
                        val = p;
                        p += strlen(p);
                    } else if (i < argc) {
                        const char *nxt = argv[i];
                        if (strcmp(nxt, "--") != 0 &&
                            (nxt[0] != '-' || cargs_token_looks_numeric(nxt))) {
                            val = argv[i++]; /* separated: -l 12  or  -l -12 */
                        }
                        // else: leave val=NULL (no value), so just '-l' toggles
                        // it
                    }
                }
                int rc = o->cb ? o->cb(val, user) : CARGS_OK;
                if (rc < 0) return rc;
                if (o->group && o->group < MAXG) counts[o->group]++;
            }
            continue;
        }
    }

    /* Enforce group policies */
    for (uint8_t g = 1; g < MAXG; g++) {
        if ((xor_mask & (1ull << g)) && counts[g] > 1) {
            fprintf(
                cargs_err(env),
                "Options in group %u are mutually exclusive (choose at most "
                "one)\n",
                g
            );
            return CARGS_ERR_GROUP;
        }
        if ((req_mask & (1ull << g)) && counts[g] != 1) {
            fprintf(
                cargs_err(env),
                "Exactly one option from group %u is required\n", g
            );
            return CARGS_ERR_GROUP;
        }
    }

    *idx = i;
    return CARGS_OK;
}

static inline int cargs_dispatch(
    const cargs_env *env, const cargs_cmd *root, int argc, char **argv,
    void *user
) {
    if (!root || !argv || argc <= 0) return CARGS_ERR_BAD_FORMAT;
    const char      *prog = argv[0];
    const cargs_cmd *cmd  = root;
    const char      *path[16];
    size_t           depth = 0; /* fixed max depth to keep zero-alloc */

    int              i     = 1;
    while (1) {
        /* Parse this level's options */
        int rc = cargs_parse_opts_level(
            env, cmd, argc, argv, &i, user,
            /*allow_version*/ depth == 0, /*allow_author*/ depth == 0, prog,
            path, depth
        );
        if (rc == CARGS_DONE) return CARGS_OK; /* help/version printed */
        if (rc < 0) return rc;

        if (i >= argc) break;                  /* nothing else */
        if (argv[i][0] == '-') break;          /* positional at this level */

        /* Try to descend into a subcommand */
        const cargs_cmd *sub = cargs_find_sub(cmd, argv[i]);
        if (!sub)
            break; /* not a subcommand — treat as positional for current cmd */
        if (depth < (sizeof(path) / sizeof(path[0]))) {
            path[depth++] = argv[i];
        }
        i++;
        cmd = sub; /* next iteration: parse subcommand level */
    }

    /* We are at the deepest matched command; validate positionals */
    int pos_rc = cargs_validate_positional(env, cmd, argc - i, &argv[i]);
    if (pos_rc < 0) return pos_rc;

    /* Run */
    if (cmd->run) { return cmd->run(argc - i, &argv[i], user); }

    /* No runner: show help for this command. */
    cargs_print_help(env, cmd, prog, path, depth);
    return CARGS_OK;
}

/* ===== Typed helpers ===== */
static inline int cargs_read_int(const char *s, int *out) {
    if (!s || !*s || !out) return -1;
    errno     = 0;
    char *end = NULL;
    long  v   = strtol(s, &end, 0);
    if (errno || end == s || *end != '\0') return -1;
    if (v < INT_MIN || v > INT_MAX) return -1;
    *out = (int)v;
    return 0;
}
static inline int cargs_read_uint64(const char *s, uint64_t *out) {
    if (!s || !*s || !out) return -1;
    errno                  = 0;
    char              *end = NULL;
    unsigned long long v   = strtoull(s, &end, 0);
    if (errno || end == s || *end != '\0') return -1;
    *out = (uint64_t)v;
    return 0;
}
static inline int cargs_read_size(
    const char *s, uint64_t *out, bool prefer_iec
) {
    if (!s || !*s || !out) return -1;
    errno      = 0;
    char  *end = NULL;
    double v   = strtod(s, &end);
    if (end == s) return -1;
    while (*end == ' ' || *end == '\t') end++;
    double mult = 1.0;
    if (*end) {
        char   a   = (char)toupper((unsigned char)end[0]);
        char   b   = (char)toupper((unsigned char)end[1]);
        char   c   = (char)toupper((unsigned char)end[2]);
        double k10 = 1000.0, k2 = 1024.0;
        switch (a) {
            case 'B': end++; break;
            case 'K':
                mult = (b == 'I') ? k2 : (prefer_iec ? k2 : k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            case 'M':
                mult =
                    (b == 'I') ? k2 * k2 : (prefer_iec ? k2 * k2 : k10 * k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            case 'G':
                mult = (b == 'I')
                           ? k2 * k2 * k2
                           : (prefer_iec ? k2 * k2 * k2 : k10 * k10 * k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            case 'T':
                mult = (b == 'I') ? k2 * k2 * k2 * k2
                                  : (prefer_iec ? k2 * k2 * k2 * k2
                                                : k10 * k10 * k10 * k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            case 'P':
                mult = (b == 'I') ? k2 * k2 * k2 * k2 * k2
                                  : (prefer_iec ? k2 * k2 * k2 * k2 * k2
                                                : k10 * k10 * k10 * k10 * k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            case 'E':
                mult = (b == 'I')
                           ? k2 * k2 * k2 * k2 * k2 * k2
                           : (prefer_iec ? k2 * k2 * k2 * k2 * k2 * k2
                                         : k10 * k10 * k10 * k10 * k10 * k10);
                end += (b == 'I' && (c == 'B' || c == '\0'))
                           ? ((c == 'B') ? 3 : 2)
                           : ((b == 'B') ? 2 : 1);
                break;
            default: return -1;
        }
        while (*end == ' ' || *end == '\t') end++;
        if (*end != '\0') return -1;
    }
    double bytes = v * mult;
    if (bytes < 0.0 || bytes > (double)UINT64_MAX) return -1;
    *out = (uint64_t)(bytes + 0.5);
    return 0;
}
static inline int cargs_read_size_si(const char *s, uint64_t *out) {
    return cargs_read_size(s, out, false);
}
static inline int cargs_read_size_iec(const char *s, uint64_t *out) {
    return cargs_read_size(s, out, true);
}
static inline char *cargs_fmt_bytes(
    uint64_t bytes, char *buf, size_t bufsz, bool iec, int decimals
) {
    if (!buf || bufsz == 0) return buf;

    static const char *U_SI[]  = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    static const char *U_IEC[] = {"B",   "KiB", "MiB", "GiB",
                                  "TiB", "PiB", "EiB"};
    const char       **U       = iec ? U_IEC : U_SI;
    const double       k       = iec ? 1024.0 : 1000.0;
    double             v       = (double)bytes;
    size_t             i       = 0;
    while (v >= k && i < 6) {
        v /= k;
        i++;
    }

    if (decimals < 0) decimals = 2;
    if (decimals > 6) decimals = 6;

    int n;
    if (i == 0 || v >= 10.0 || decimals == 0) {
        n = snprintf(buf, bufsz, "%.0f%s", v, U[i]);
    } else {
        n = snprintf(buf, bufsz, "%.*f%s", decimals, v, U[i]);
    }

    if (n < 0 || (size_t)n >= bufsz) { buf[bufsz ? bufsz - 1 : 0] = '\0'; }
    return buf;
}

/* ===== Documentation Emitters ===== */
static inline void cargs__emit_md(
    const cargs_env *env, const cargs_cmd *cmd, const char *prog,
    const char *const *path, size_t depth, FILE *out
) {
    fprintf(out, "%s# ", depth == 0 ? "" : "\n");
    fprintf(out, "%s", prog);
    for (size_t i = 0; i < depth; i++) fprintf(out, " %s", path[i]);
    fputc('\n', out);

    char posbuf[192];
    cargs_render_pos_usage(posbuf, sizeof(posbuf), cmd);

    fprintf(out, "\n**Usage:** `");
    fprintf(out, "%s", prog);
    for (size_t i = 0; i < depth; i++) fprintf(out, " %s", path[i]);
    if (cargs__has_any_options(env, cmd)) fputs(" [options]", out);
    if (cmd && cmd->sub_count) fputs(" <command> [command-options]", out);
    fputs(posbuf, out);
    fputs("`\n\n", out);

    if (env && env->auto_help) {
        fprintf(out, "### Options\n- `-h, --help` — Show this help and exit\n");
    }
    if (depth == 0 && env && env->auto_version && env->version) {
        fprintf(out, "- `-v, --version` — Show version and exit\n");
    }
    if (depth == 0 && env && env->auto_author && env->author) {
        fprintf(out, "- `--author` — Show author and exit\n");
    }
    if (cmd && cmd->opt_count) {
        for (size_t i = 0; i < cmd->opt_count; i++) {
            const cargs_opt *o = &cmd->opts[i];
            fprintf(out, "- `");
            if (o->short_name) {
                fprintf(out, "-%c", o->short_name);
                if (o->long_name) fputs(", ", out);
            }
            if (o->long_name) fprintf(out, "--%s", o->long_name);
            if (o->arg == CARGS_ARG_REQUIRED)
                fprintf(out, " %s", o->metavar ? o->metavar : "VALUE");
            if (o->arg == CARGS_ARG_OPTIONAL)
                fprintf(out, " [%s]", o->metavar ? o->metavar : "VALUE");
            fprintf(out, "` — %s\n", o->help ? o->help : "");
        }
    }

    if (cmd && cmd->pos_count) {
        fprintf(out, "\n### Positionals\n");
        for (size_t i = 0; i < cmd->pos_count; i++) {
            const cargs_pos *p = &cmd->pos[i];
            if (!p->name) continue;

            fprintf(out, "- **%s**", p->name);
            if (p->desc && *p->desc) fprintf(out, " — %s", p->desc);

            if (p->min != 1 || p->max != 1) {
                if (p->min == p->max) {
                    fprintf(out, " (x%u)", (unsigned)p->min);
                } else if (p->max == CARGS_POS_INF) {
                    fprintf(out, " (%u..inf)", (unsigned)p->min);
                } else {
                    fprintf(
                        out, " (%u..%u)", (unsigned)p->min, (unsigned)p->max
                    );
                }
            }
            fputc('\n', out);
        }
    }

    if (cmd && cmd->sub_count) {
        fprintf(out, "\n### Commands\n");
        for (size_t i = 0; i < cmd->sub_count; i++) {
            const cargs_cmd *c = &cmd->subs[i];
            fprintf(out, "- **%s**", c->name ? c->name : "");
            if (c->alias_count) {
                fputs(" (alias: ", out);
                for (size_t a = 0; a < c->alias_count; a++) {
                    if (a) fputs(", ", out);
                    fputs(c->aliases[a], out);
                }
                fputc(')', out);
            }
            if (c->desc) fprintf(out, " — %s", c->desc);
            fputc('\n', out);
        }
        for (size_t i2 = 0; i2 < cmd->sub_count; i2++) {
            const cargs_cmd *c = &cmd->subs[i2];
            const char      *newpath[16];
            for (size_t k = 0; k < depth; k++) newpath[k] = path[k];
            newpath[depth] = c->name;
            cargs__emit_md(env, c, prog, newpath, depth + 1, out);
        }
    }
}

static inline void cargs_emit_markdown(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
) {
    (void)env;
    if (!out) out = stdout;
    const char *empty[1] = {0};
    cargs__emit_md(env, root, prog, empty, 0, out);
}

static inline void cargs__emit_man(
    const cargs_env *env, const cargs_cmd *cmd, const char *prog,
    const char *const *path, size_t depth, FILE *out
) {
    (void)env;
    fprintf(out, "\n.SH NAME\n%s", prog);
    for (size_t i = 0; i < depth; i++) fprintf(out, " %s", path[i]);
    fputs(" - ", out);
    if (cmd && cmd->desc) fputs(cmd->desc, out);
    fputc('\n', out);
    fprintf(out, ".SH SYNOPSIS\n\fB%s\fR", prog);
    for (size_t i = 0; i < depth; i++) fprintf(out, " %s", path[i]);
    fputs(" [options]", out);
    if (cmd && cmd->sub_count) fputs(" <command> [command-options]", out);
    char pbuf[192];
    cargs_render_pos_usage(pbuf, sizeof(pbuf), cmd);
    fputs(pbuf, out);
    fputs("\n", out);
    if (cmd && (cmd->opt_count || (depth == 0))) {
        fputs(".SH OPTIONS\n", out);
        if (depth == 0) {
            fputs(".TP\n\fB-h, --help\fR\nShow this help and exit\n", out);
            if (env && env->version)
                fputs(".TP\n\fB-v, --version\fR\nShow version and exit\n", out);
            if (env && env->author)
                fputs(".TP\n\fB--author\fR\nShow author and exit\n", out);
        }
        for (size_t i = 0; i < cmd->opt_count; i++) {
            const cargs_opt *o = &cmd->opts[i];
            fputs(".TP\n\fB", out);
            if (o->short_name) {
                fprintf(out, "-%c", o->short_name);
                if (o->long_name) fputs(", ", out);
            }
            if (o->long_name) { fprintf(out, "--%s", o->long_name); }
            if (o->arg == CARGS_ARG_REQUIRED)
                fprintf(out, " %s", o->metavar ? o->metavar : "VALUE");
            if (o->arg == CARGS_ARG_OPTIONAL)
                fprintf(out, "[%s]", o->metavar ? o->metavar : "VALUE");
            fputs("\fR\n", out);
            if (o->help) fputs(o->help, out);
            fputc('\n', out);
        }
    }
    if (cmd && cmd->pos_count) {
        fputs(".SH POSITIONALS\n", out);
        for (size_t i = 0; i < cmd->pos_count; i++) {
            const cargs_pos *p = &cmd->pos[i];
            fputs(".TP\n\fB", out);
            fputs(p->name ? p->name : "ARG", out);
            fputs("\fR\nOccurrences: ", out);
            if (p->min == p->max) fprintf(out, "%u\n", (unsigned)p->min);
            else if (p->max == CARGS_POS_INF)
                fprintf(out, "%u..inf\n", (unsigned)p->min);
            else fprintf(out, "%u..%u\n", (unsigned)p->min, (unsigned)p->max);
        }
    }
    if (cmd && cmd->sub_count) {
        fputs(".SH COMMANDS\n", out);
        for (size_t i = 0; i < cmd->sub_count; i++) {
            const cargs_cmd *c = &cmd->subs[i];
            fputs(".TP\n\fB", out);
            fputs(c->name ? c->name : "", out);
            if (c->alias_count) {
                fputs("\fR (alias: ", out);
                for (size_t a = 0; a < c->alias_count; a++) {
                    if (a) fputs(", ", out);
                    fputs(c->aliases[a], out);
                }
                fputs(")\n", out);
            } else fputs("\n", out);
            if (c->desc) fputs(c->desc, out);
            fputc('\n', out);
        }
        for (size_t j = 0; j < cmd->sub_count; j++) {
            const cargs_cmd *c = &cmd->subs[j];
            const char      *newpath[16];
            for (size_t k = 0; k < depth; k++) newpath[k] = path[k];
            newpath[depth] = c->name;
            cargs__emit_man(env, c, prog, newpath, depth + 1, out);
        }
    }
}

static inline void cargs_emit_man(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out,
    const char *section
) {
    if (!out) out = stdout;
    if (!section) section = "1";
    fprintf(out, ".TH %s %s\n", prog, section);
    const char *empty[1] = {0};
    cargs__emit_man(env, root, prog, empty, 0, out);
}

/* ===== Completion Generators ===== */
static inline void cargs__emit_bash_level(
    const cargs_env *env, const cargs_cmd *cmd, const char *pathname,
    const char *prog, FILE *out
) {
    (void)env;
    (void)prog;
    fprintf(
        out, "  case \"$scope\" in\n    %s)\n      candidates=\"", pathname
    );
    if (cmd) {
        for (size_t i = 0; i < cmd->opt_count; i++) {
            const cargs_opt *o = &cmd->opts[i];
            if (o->long_name) { fprintf(out, "--%s ", o->long_name); }
            if (o->short_name) { fprintf(out, "-%c ", o->short_name); }
        }
    }
    fputs("--help -h ", out);
    if (cmd) {
        for (size_t i = 0; i < cmd->sub_count; i++) {
            const cargs_cmd *c = &cmd->subs[i];
            if (c->name) { fprintf(out, "%s ", c->name); }
            for (size_t a = 0; a < c->alias_count; a++) {
                fprintf(out, "%s ", c->aliases[a]);
            }
        }
    }
    fputs("\"\n      ;;\n  esac\n", out);
}

static inline void cargs__emit_bash_tree(
    const cargs_env *env, const cargs_cmd *cmd, const char *prog,
    const char *path_so_far, FILE *out
) {
    char path[256];
    snprintf(path, sizeof(path), "%s", path_so_far);
    cargs__emit_bash_level(env, cmd, path[0] ? path : "root", prog, out);
    for (size_t i = 0; i < cmd->sub_count; i++) {
        char child[256];
        if (path[0])
            snprintf(
                child, sizeof(child), "%s_%s", path,
                cmd->subs[i].name ? cmd->subs[i].name : ""
            );
        else
            snprintf(
                child, sizeof(child), "%s",
                cmd->subs[i].name ? cmd->subs[i].name : ""
            );
        cargs__emit_bash_tree(env, &cmd->subs[i], prog, child, out);
    }
}

static inline void cargs_emit_completion_bash(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
) {
    if (!out) out = stdout;
    if (!prog) prog = "prog";
    fprintf(
        out,
        "_%s_complete() {\n"
        "  local cur prev words cword\n"
        "  _init_completion -n : 2>/dev/null || {\n"
        "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
        "  }\n"
        "  local scope=root\n"
        "  local i=1 w\n"
        "  while [[ $i -lt ${#COMP_WORDS[@]} ]]; do\n"
        "    w=${COMP_WORDS[$i]}\n"
        "    case \"$scope\" in\n"
        "      root)\n",
        prog
    );

    /* root resolver */
    fprintf(out, "        case \"$w\" in\n");
    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (c->name)
            fprintf(out, "          %s) scope=%s; ;;\n", c->name, c->name);
        for (size_t a = 0; a < c->alias_count; a++)
            fprintf(
                out, "          %s) scope=%s; ;;\n", c->aliases[a],
                c->name ? c->name : ""
            );
    }
    fprintf(
        out,
        "          --*) ;;\n          *) break ;;\n        esac\n        ;;\n"
    );

    /* nested resolvers */
    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (!c->name) continue;
        fprintf(out, "      %s)\n        case \"$w\" in\n", c->name);
        for (size_t j = 0; j < c->sub_count; j++) {
            const cargs_cmd *s = &c->subs[j];
            if (!s->name) continue;
            fprintf(
                out, "          %s) scope=%s_%s; ;;\n", s->name, c->name,
                s->name
            );
            for (size_t a = 0; a < s->alias_count; a++)
                fprintf(
                    out, "          %s) scope=%s_%s; ;;\n", s->aliases[a],
                    c->name, s->name
                );
        }
        fprintf(
            out,
            "          --*) ;;\n          *) break ;;\n        esac\n        "
            ";;\n"
        );
    }

    fputs("    esac\n    i=$((i+1))\n  done\n\n  local candidates=\"\"\n", out);

    cargs__emit_bash_tree(env, root, prog, "", out);

    fputs(
        "  COMPREPLY=( $(compgen -W \"$candidates\" -- \"$cur\") )\n}\n", out
    );
    fprintf(out, "complete -F _%s_complete %s\n", prog, prog);
}

static inline void cargs_emit_completion_zsh(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
) {
    (void)env;
    if (!out) out = stdout;
    if (!prog) prog = "prog";
    fprintf(
        out,
        "#compdef %s\n\n_%s() {\n  local -a candidates\n  local context state "
        "state_descr line\n  typeset -A opt_args\n  local -a "
        "words=(${=words})\n  local cur=${words[-1]}\n  local scope=root\n  "
        "local i=2 w\n  while (( i <= $#words )); do\n    w=${words[i]}\n    "
        "case $scope in\n      root)\n",
        prog, prog
    );

    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (c->name)
            fprintf(
                out, "        [[ $w == %s ]] && scope=%s\n", c->name, c->name
            );
        for (size_t a = 0; a < c->alias_count; a++)
            fprintf(
                out, "        [[ $w == %s ]] && scope=%s\n", c->aliases[a],
                c->name ? c->name : ""
            );
    }
    fputs("        ;;\n", out);

    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (!c->name) continue;
        fprintf(out, "      %s)\n", c->name);
        for (size_t j = 0; j < c->sub_count; j++) {
            const cargs_cmd *s = &c->subs[j];
            if (!s->name) continue;
            fprintf(
                out, "        [[ $w == %s ]] && scope=%s_%s\n", s->name,
                c->name, s->name
            );
            for (size_t a = 0; a < s->alias_count; a++)
                fprintf(
                    out, "        [[ $w == %s ]] && scope=%s_%s\n",
                    s->aliases[a], c->name, s->name
                );
        }
        fputs("        ;;\n", out);
    }

    fputs("    esac\n    (( i++ ))\n  done\n\n  case $scope in\n", out);

    /* root candidates */
    fputs("  root)\n    candidates=(", out);
    if (root) {
        for (size_t i = 0; i < root->opt_count; i++) {
            const cargs_opt *o = &root->opts[i];
            if (o->long_name) { fprintf(out, "--%s ", o->long_name); }
            if (o->short_name) { fprintf(out, "-%c ", o->short_name); }
        }
    }
    fputs("--help -h ", out);
    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (c->name) fprintf(out, "%s ", c->name);
        for (size_t a = 0; a < c->alias_count; a++)
            fprintf(out, "%s ", c->aliases[a]);
    }
    fputs(")\n    compadd $candidates\n    ;;\n", out);

    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        if (!c->name) continue;
        fprintf(out, "  %s)\n    candidates=(", c->name);
        for (size_t k = 0; k < c->opt_count; k++) {
            const cargs_opt *o = &c->opts[k];
            if (o->long_name) { fprintf(out, "--%s ", o->long_name); }
            if (o->short_name) { fprintf(out, "-%c ", o->short_name); }
        }
        fputs("--help -h ", out);
        for (size_t j = 0; j < c->sub_count; j++) {
            const cargs_cmd *s = &c->subs[j];
            if (s->name) fprintf(out, "%s ", s->name);
            for (size_t a = 0; a < s->alias_count; a++)
                fprintf(out, "%s ", s->aliases[a]);
        }
        fputs(")\n    compadd $candidates\n    ;;\n", out);
    }

    fputs("  esac\n}\n\ncompdef _", out);
    fputs(prog, out);
    fputs(" ", out);
    fputs(prog, out);
    fputc('\n', out);
}

static inline void cargs_emit_completion_fish(
    const cargs_env *env, const cargs_cmd *root, const char *prog, FILE *out
) {
    if (!out) out = stdout;
    if (!prog) prog = "prog";
    (void)env;
    /* Root subcommands */
    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        fprintf(
            out, "complete -c %s -n '__fish_use_subcommand' -a '%s' -d '%s'\n",
            prog, c->name ? c->name : "", c->desc ? c->desc : ""
        );
        for (size_t a = 0; a < c->alias_count; a++) {
            fprintf(
                out,
                "complete -c %s -n '__fish_use_subcommand' -a '%s' -d '%s'\n",
                prog, c->aliases[a], c->desc ? c->desc : ""
            );
        }
    }
    /* Root options */
    for (size_t i = 0; i < root->opt_count; i++) {
        const cargs_opt *o = &root->opts[i];
        if (o->long_name)
            fprintf(
                out, "complete -c %s -l %s -d '%s'\n", prog, o->long_name,
                o->help ? o->help : ""
            );
        if (o->short_name)
            fprintf(
                out, "complete -c %s -s %c -d '%s'\n", prog, o->short_name,
                o->help ? o->help : ""
            );
    }
    fprintf(out, "complete -c %s -s h -l help -d 'Show help'\n", prog);
    /* Nested subcommands */
    for (size_t i = 0; i < root->sub_count; i++) {
        const cargs_cmd *c = &root->subs[i];
        for (size_t j = 0; j < c->sub_count; j++) {
            const cargs_cmd *s = &c->subs[j];
            fprintf(
                out,
                "complete -c %s -n '__fish_seen_subcommand_from %s' -a '%s' -d "
                "'%s'\n",
                prog, c->name ? c->name : "", s->name ? s->name : "",
                s->desc ? s->desc : ""
            );
        }
        for (size_t k = 0; k < c->opt_count; k++) {
            const cargs_opt *o = &c->opts[k];
            if (o->long_name)
                fprintf(
                    out,
                    "complete -c %s -n '__fish_seen_subcommand_from %s' -l %s "
                    "-d '%s'\n",
                    prog, c->name ? c->name : "", o->long_name,
                    o->help ? o->help : ""
                );
            if (o->short_name)
                fprintf(
                    out,
                    "complete -c %s -n '__fish_seen_subcommand_from %s' -s %c "
                    "-d '%s'\n",
                    prog, c->name ? c->name : "", o->short_name,
                    o->help ? o->help : ""
                );
        }
    }
}

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* CARGS_H */
