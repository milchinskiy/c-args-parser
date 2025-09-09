#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c-args-parser.h"

#ifdef _WIN32
#    define CARGS_DEVNULL "NUL"
#else
#    define CARGS_DEVNULL "/dev/null"
#endif

static FILE *cargs_devnull(void) {
    static FILE *f = NULL;
    if (!f) {
        f = fopen(CARGS_DEVNULL, "w");
        if (!f) f = stdout;
    }
    return f;
}

/* default: quiet unless CARGS_TEST_VERBOSE is set */
static int cargs_quiet(void) {
    const char *v = getenv("CARGS_TEST_VERBOSE");
    return !(v && *v); /* quiet when unset/empty */
}

static int tests_run = 0;
static int failures  = 0;

#define CHECK(cond)                                                           \
    do {                                                                      \
        tests_run++;                                                          \
        if (!(cond)) {                                                        \
            failures++;                                                       \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                     \
    } while (0)

#define CHECK_EQI(got, exp)                                                                                      \
    do {                                                                                                         \
        tests_run++;                                                                                             \
        if ((got) != (exp)) {                                                                                    \
            failures++;                                                                                          \
            fprintf(                                                                                             \
                stderr, "[FAIL] %s:%d: %s == %s  (got=%d exp=%d)\n", __FILE__, __LINE__, #got, #exp, (int)(got), \
                (int)(exp)                                                                                       \
            );                                                                                                   \
        }                                                                                                        \
    } while (0)

#define CHECK_STREQ(got, exp)                                                                             \
    do {                                                                                                  \
        tests_run++;                                                                                      \
        const char *_g = (got);                                                                           \
        const char *_e = (exp);                                                                           \
        if (((_g) && (_e) && strcmp(_g, _e) != 0) || (((_g) == NULL) ^ ((_e) == NULL))) {                 \
            failures++;                                                                                   \
            fprintf(                                                                                      \
                stderr, "[FAIL] %s:%d: strings differ\n  got=\"%s\"\n  exp=\"%s\"\n", __FILE__, __LINE__, \
                (_g) ? _g : "(null)", (_e) ? _e : "(null)"                                                \
            );                                                                                            \
        }                                                                                                 \
    } while (0)

/* ---------- state & callbacks for tests ---------- */
typedef struct {
    int       verbose;
    long      jobs;
    int       has_limit;
    long long limit;
    int       json, yaml;
    int       mode;
    int       ran_root, ran_remote_add, ran_remote_rm;

    int       pos_argc;
    char     *pos_argv[8];
} tstate;

static void tstate_clear(tstate *s) {
    for (int i = 0; i < s->pos_argc && i < 8; i++) {
        free(s->pos_argv[i]);
        s->pos_argv[i] = NULL;
    }
    s->pos_argc = 0;
}

/* tiny, portable strdup to avoid relying on POSIX */
static char *dup_cstr(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char  *p = (char *)malloc(n);
    if (!p) {
        perror("malloc");
        abort();
    }
    memcpy(p, s, n);
    return p;
}

static int cb_verbose(const char *v, void *u) {
    (void)v;
    ((tstate *)u)->verbose = 1;
    return CARGS_OK;
}
static int cb_jobs(const char *v, void *u) {
    if (!v) return CARGS_ERR_MISSING_VAL;
    char *end = NULL;
    long  x   = strtol(v, &end, 10);
    if (end == v || *end != '\0') return CARGS_ERR_BAD_FORMAT;
    ((tstate *)u)->jobs = x;
    return CARGS_OK;
}
static int cb_limit_opt(const char *v, void *u) {
    tstate *s    = (tstate *)u;
    s->has_limit = 1;
    if (!v) {
        s->limit = LLONG_MIN;
        return CARGS_OK;
    } /* bare -l/--limit */
    char     *end = NULL;
    long long x   = strtoll(v, &end, 10);
    if (end == v || *end != '\0') return CARGS_ERR_BAD_FORMAT;
    s->limit = x;
    return CARGS_OK;
}
static int cb_json(const char *v, void *u) {
    (void)v;
    ((tstate *)u)->json = 1;
    return CARGS_OK;
}
static int cb_yaml(const char *v, void *u) {
    (void)v;
    ((tstate *)u)->yaml = 1;
    return CARGS_OK;
}
static int cb_light(const char *v, void *u) {
    (void)v;
    ((tstate *)u)->mode = 1;
    return CARGS_OK;
}
static int cb_dark(const char *v, void *u) {
    (void)v;
    ((tstate *)u)->mode = 2;
    return CARGS_OK;
}

/* runners */
static int run_root(int argc, char **argv, void *user) {
    tstate *s   = (tstate *)user;
    s->ran_root = 1;
    tstate_clear(s);
    s->pos_argc = argc;
    for (int i = 0; i < argc && i < 8; i++) s->pos_argv[i] = dup_cstr(argv[i]);
    return CARGS_OK;
}
static int run_remote_add(int argc, char **argv, void *user) {
    tstate *s         = (tstate *)user;
    s->ran_remote_add = 1;
    tstate_clear(s);
    s->pos_argc = argc;
    for (int i = 0; i < argc && i < 8; i++) s->pos_argv[i] = dup_cstr(argv[i]);
    return CARGS_OK;
}
static int run_remote_rm(int argc, char **argv, void *user) {
    tstate *s        = (tstate *)user;
    s->ran_remote_rm = 1;
    tstate_clear(s);
    s->pos_argc = argc;
    for (int i = 0; i < argc && i < 8; i++) s->pos_argv[i] = dup_cstr(argv[i]);
    return CARGS_OK;
}

/* ---------- command trees for different test groups ---------- */

/* basic root with -V/--verbose, -j/--jobs (REQUIRED), -l/--limit (OPTIONAL),
   XOR group: --json/--yaml, subcommands: remote add/remove (positional) */
static void build_root_basic(const cargs_cmd **out_root) {
    static const char     *remote_alias_rm[] = {"rm"};
    static const cargs_pos pos_add[]         = {
        CARGS_POS("NAME", NULL),
        CARGS_POS("URL", NULL),
    };
    static const cargs_pos pos_rm[] = {
        CARGS_POS("NAME", NULL),
    };

    static const cargs_opt root_opts[] = {
        {"verbose", 'V', CARGS_ARG_NONE,     NULL, "verbose",          cb_verbose,   NULL, NULL, 0, CARGS_GRP_NONE},
        {"jobs",    'j', CARGS_ARG_REQUIRED, "N",  "jobs",             cb_jobs,      NULL, NULL, 0, CARGS_GRP_NONE},
        {"limit",   'l', CARGS_ARG_OPTIONAL, "N",  "limit (optional)", cb_limit_opt, NULL, NULL, 0, CARGS_GRP_NONE},
        {"json",    0,   CARGS_ARG_NONE,     NULL, "json",             cb_json,      NULL, NULL, 1, CARGS_GRP_XOR },
        {"yaml",    0,   CARGS_ARG_NONE,     NULL, "yaml",             cb_yaml,      NULL, NULL, 1, CARGS_GRP_XOR },
    };

    static const cargs_cmd remote_subs[] = {
        {.name        = "add",
         .desc        = "Add",
         .pos         = pos_add,
         .pos_count   = 2,
         .run         = run_remote_add},
        {.name        = "remove",
         .desc        = "Remove",
         .aliases     = remote_alias_rm,
         .alias_count = 1,
         .pos         = pos_rm,
         .pos_count   = 1,
         .run         = run_remote_rm },
    };
    static const cargs_cmd remote = {
        .name        = "remote",
        .desc        = "Manage remotes",
        .subs        = remote_subs,
        .sub_count   = sizeof(remote_subs) / sizeof(remote_subs[0]),
    };
    static const cargs_cmd subs[] = {remote};

    static const cargs_cmd root   = {
          .opts        = root_opts,
          .opt_count   = sizeof(root_opts) / sizeof(root_opts[0]),
          .subs        = subs,
          .sub_count   = sizeof(subs) / sizeof(subs[0]),
          .run         = run_root
    };
    *out_root = &root;
}

/* variant that requires exactly one of --light/--dark (REQ_ONE group) */
static void build_root_req_one(const cargs_cmd **out_root) {
    static const cargs_opt opts[] = {
        {"light", 0, CARGS_ARG_NONE, NULL, "light mode", cb_light, NULL, NULL, 2, CARGS_GRP_REQ_ONE},
        {"dark",  0, CARGS_ARG_NONE, NULL, "dark mode",  cb_dark,  NULL, NULL, 2, CARGS_GRP_REQ_ONE},
    };
    static const cargs_cmd root = {
        .opts        = opts,
        .opt_count   = sizeof(opts) / sizeof(opts[0]),
        .run         = run_root
    };
    *out_root = &root;
}

/* shared env */
static void fill_env(cargs_env *env) {
    env->prog         = "t";
    env->version      = "v1";
    env->author       = "a";
    env->auto_help    = true;
    env->auto_version = true;
    env->auto_author  = true;
    env->wrap_cols    = 80;
    env->color        = false;

    if (cargs_quiet()) {
        FILE *dn = cargs_devnull();
        env->out = dn; /* parser help/info */
        env->err = dn; /* parser diagnostics ("Unknown option", etc.) */
    } else {
        env->out = stdout;
        env->err = stderr;
    }
}

/* helper to run one argv vector */
static int run_vec(const cargs_cmd *root, cargs_env *env, tstate *st, int argc, const char **argv0) {
    size_t n    = (size_t)argc;
    char **argv = (char **)malloc(n * sizeof(*argv));
    if (!argv) {
        perror("malloc");
        abort();
    }

    for (size_t i = 0; i < n; i++) { argv[i] = dup_cstr(argv0[i]); /* no cast-away const; strdup-like */ }
    int rc = cargs_dispatch(env, root, (int)n, argv, st);
    for (size_t i = 0; i < n; i++) free(argv[i]);
    free(argv);
    return rc;
}

/* ---------- individual tests ---------- */

static void test_required_forms(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* --jobs 10 */
    const char *a1[] = {"t", "--jobs", "10"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a1), CARGS_OK);
    CHECK_EQI(st.jobs, 10);

    /* --jobs=10 */
    st               = (tstate){0};
    const char *a2[] = {"t", "--jobs=10"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a2), CARGS_OK);
    CHECK_EQI(st.jobs, 10);

    /* -j10 */
    st               = (tstate){0};
    const char *a3[] = {"t", "-j10"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a3), CARGS_OK);
    CHECK_EQI(st.jobs, 10);

    /* -j 10 */
    st               = (tstate){0};
    const char *a4[] = {"t", "-j", "10"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a4), CARGS_OK);
    CHECK_EQI(st.jobs, 10);

    /* -j -10 (negative should be accepted) */
    st               = (tstate){0};
    const char *a5[] = {"t", "-j", "-10"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a5), CARGS_OK);
    CHECK_EQI(st.jobs, -10);
}

static void test_optional_forms(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* -l12 */
    const char *a1[] = {"t", "-l12"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a1), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, 12);

    /* -l 12 */
    st               = (tstate){0};
    const char *a2[] = {"t", "-l", "12"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a2), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, 12);

    /* -l -12 (numeric-looking negative) */
    st               = (tstate){0};
    const char *a3[] = {"t", "-l", "-12"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a3), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, -12);

    /* bare -l (present but no value) */
    st               = (tstate){0};
    const char *a4[] = {"t", "-l"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a4), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, LLONG_MIN);

    /* --limit=34 */
    st               = (tstate){0};
    const char *a5[] = {"t", "--limit=34"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a5), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, 34);

    /* --limit -5 */
    st               = (tstate){0};
    const char *a6[] = {"t", "--limit", "-5"};
    CHECK_EQI(run_vec(root, &env, &st, 3, a6), CARGS_OK);
    CHECK(st.has_limit);
    CHECK_EQI(st.limit, -5);
}

static void test_unknown_and_missing(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* unknown short */
    const char *u1[] = {"t", "-Z"};
    CHECK(run_vec(root, &env, &st, 2, u1) == CARGS_ERR_UNKNOWN);

    /* required missing value */
    const char *u2[] = {"t", "-j"};
    CHECK(run_vec(root, &env, &st, 2, u2) == CARGS_ERR_MISSING_VAL);

    /* short with = (should be bad format via callback) */
    const char *u3[] = {"t", "-j=10"};
    CHECK(run_vec(root, &env, &st, 2, u3) != CARGS_OK);

    /* long required without value */
    const char *u4[] = {"t", "--jobs"};
    CHECK(run_vec(root, &env, &st, 2, u4) == CARGS_ERR_MISSING_VAL);
}

static void test_group_xor(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* choose one */
    const char *a1[] = {"t", "--json"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a1), CARGS_OK);
    CHECK_EQI(st.json, 1);
    CHECK_EQI(st.yaml, 0);

    /* choose both -> error */
    st               = (tstate){0};
    const char *a2[] = {"t", "--json", "--yaml"};
    CHECK(run_vec(root, &env, &st, 3, a2) == CARGS_ERR_GROUP);
}

static void test_group_req_one(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_req_one(&root);
    tstate st = {0};

    /* none -> error */
    const char *a1[] = {"t"};
    CHECK(run_vec(root, &env, &st, 1, a1) == CARGS_ERR_GROUP);

    /* exactly one */
    const char *a2[] = {"t", "--light"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a2), CARGS_OK);
    CHECK_EQI(st.mode, 1);

    st               = (tstate){0};
    const char *a3[] = {"t", "--dark"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a3), CARGS_OK);
    CHECK_EQI(st.mode, 2);
}

static void test_double_dash_and_positionals(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* after -- we collect positionals, even if they look like options */
    const char *a1[] = {"t", "--", "-not", "--an", "option"};
    CHECK_EQI(run_vec(root, &env, &st, 5, a1), CARGS_OK);
    CHECK_EQI(st.ran_root, 1);
    CHECK_EQI(st.pos_argc, 3);
    CHECK_STREQ(st.pos_argv[0], "-not");
    CHECK_STREQ(st.pos_argv[1], "--an");
    CHECK_STREQ(st.pos_argv[2], "option");
    tstate_clear(&st);
}

static void test_subcommands_and_aliases(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* remote add NAME URL */
    const char *a1[] = {"t", "remote", "add", "origin", "git@git.co"};
    CHECK_EQI(run_vec(root, &env, &st, 5, a1), CARGS_OK);
    CHECK_EQI(st.ran_remote_add, 1);
    CHECK_EQI(st.pos_argc, 2);
    CHECK_STREQ(st.pos_argv[0], "origin");
    CHECK_STREQ(st.pos_argv[1], "git@git.co");
    tstate_clear(&st);

    /* alias: remote rm NAME */
    st               = (tstate){0};
    const char *a2[] = {"t", "remote", "rm", "origin"};
    CHECK_EQI(run_vec(root, &env, &st, 4, a2), CARGS_OK);
    CHECK_EQI(st.ran_remote_rm, 1);
    CHECK_EQI(st.pos_argc, 1);
    CHECK_STREQ(st.pos_argv[0], "origin");
    tstate_clear(&st);

    /* missing URL -> positional error */
    st               = (tstate){0};
    const char *a3[] = {"t", "remote", "add", "origin"};
    CHECK(run_vec(root, &env, &st, 4, a3) == CARGS_ERR_POSITIONAL);
    tstate_clear(&st);
}

static void test_grouped_shorts(void) {
    cargs_env env;
    fill_env(&env);
    const cargs_cmd *root;
    build_root_basic(&root);
    tstate st = {0};

    /* -Vj10 */
    const char *a1[] = {"t", "-Vj10"};
    CHECK_EQI(run_vec(root, &env, &st, 2, a1), CARGS_OK);
    CHECK_EQI(st.verbose, 1);
    CHECK_EQI(st.jobs, 10);

    /* -V -j 3 */
    st               = (tstate){0};
    const char *a2[] = {"t", "-V", "-j", "3"};
    CHECK_EQI(run_vec(root, &env, &st, 4, a2), CARGS_OK);
    CHECK_EQI(st.verbose, 1);
    CHECK_EQI(st.jobs, 3);
}

int main(void) {
    test_required_forms();
    test_optional_forms();
    test_unknown_and_missing();
    test_group_xor();
    test_group_req_one();
    test_double_dash_and_positionals();
    test_subcommands_and_aliases();
    test_grouped_shorts();

    if (failures) {
        fprintf(stderr, "\nFAILED %d/%d checks\n", failures, tests_run);
        return 1;
    }
    printf("ok — %d checks passed\n", tests_run);
    return 0;
}
