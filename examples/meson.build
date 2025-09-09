ex_inc = include_directories('../src')

ex1 = executable('ex-hello',      ['01_hello.c'],      include_directories: ex_inc)
ex2 = executable('ex-cp-like',    ['02_cp_like.c'],    include_directories: ex_inc)
ex3 = executable('ex-remote',     ['03_remote.c'],     include_directories: ex_inc)
ex4 = executable('ex-groups',     ['04_groups.c'],     include_directories: ex_inc)
ex5 = executable('ex-docs',     ['05_docs_completion.c'],include_directories: ex_inc)
ex6 = executable('ex-env',      ['06_env_defaults.c'],    include_directories: ex_inc)
ex7 = executable('ex-sizes',    ['07_sizes.c'],           include_directories: ex_inc)
ex8 = executable('demo',        ['08_demo.c'],           include_directories: ex_inc)

# A handy alias to build all examples:
alias_target('examples', [ex1, ex2, ex3, ex4, ex5, ex6, ex7, ex8])

# Cross-platform /dev/null
devnull = host_machine.system() == 'windows' ? 'NUL' : '/dev/null'

# "Example runs" as a Meson test suite so output is shown with -v:
test('hello',   ex1, args: ['--name','Ada','--repeat','2'], suite: ['examples'])
test('cp-like', ex2, args: ['-n','src1','src2','dst/'],     suite: ['examples'])
test('remote',  ex3, args: ['remote','add','origin','git@git.co'], suite: ['examples'])
test('groups',  ex4, args: ['--json','--light'], suite: ['examples'])

# Keep doc/completion output quiet by sending to devnull
test('docs md',             ex5, args: ['--md', devnull],                 suite: ['examples'])
test('docs man',            ex5, args: ['--man', '1:' + devnull],         suite: ['examples'])
test('docs completion bash',ex5, args: ['--completion','bash:' + devnull],suite: ['examples'])
test('docs completion zsh', ex5, args: ['--completion','zsh:' + devnull], suite: ['examples'])
test('docs completion fish',ex5, args: ['--completion','fish:' + devnull],suite: ['examples'])

# Env-defaults demos
test('env defaults (env only)',     ex6, args: [], suite: ['examples'],
  env: {'JOBS':'8','OUT':'run.bin','FMT':'yaml','LEVEL':'2'})
test('env defaults (cli override)', ex6, args: ['-j','2','-o','cli.bin','--format','json','-L'],
  suite: ['examples'], env: {'JOBS':'8','OUT':'run.bin','FMT':'yaml'})

# Sizes demos
test('sizes (cli)',          ex7, args: ['--limit','256MiB','--rate','12MB','file1','file2'], suite: ['examples'])
test('sizes (env fallback)', ex7, args: ['fileA'], suite: ['examples'],
  env: {'LIMIT':'64MiB','RATE':'10MB'})

# A run target that invokes the suite verbosely:
meson_prog = find_program('meson')
run_target('examples-run',
  command: [meson_prog, 'test', '-C', meson.project_build_root(),
            '--suite', 'examples', '--verbose'],
  depends: [ex1, ex2, ex3, ex4, ex5, ex6, ex7, ex8]
)
