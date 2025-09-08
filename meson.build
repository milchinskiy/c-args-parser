project('c-args-parser', 'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true',
    'b_pie=true',
  ])

cc = meson.get_compiler('c')
pkg = import('pkgconfig')
host = host_machine.system()
san = get_option('b_sanitize')
with_san = san != 'none'

c_warn = [
  '-Wpedantic', '-Wshadow', '-Wstrict-prototypes', '-Wmissing-prototypes',
  '-Wvla', '-Wwrite-strings', '-Wcast-qual', '-Wswitch-enum',
  '-Wformat=2', '-Wformat-security', '-Wnull-dereference',
  '-Wdouble-promotion',
]
foreach w : c_warn
  if cc.has_argument(w)
    add_project_arguments(w, language: 'c')
  endif
endforeach

if cc.has_argument('-Wconversion')
  add_project_arguments('-Wconversion', language: 'c')
endif

# ---- Hardening (compile) ----
c_hard = []

if cc.has_argument('-fstack-protector-strong')
  c_hard += ['-fstack-protector-strong']
elif cc.has_argument('-fstack-protector')
  c_hard += ['-fstack-protector']
endif

if cc.has_argument('-D_FORTIFY_SOURCE=3')
  c_hard += ['-D_FORTIFY_SOURCE=3']   # glibc ≥ 2.35 (you have 2.40)
elif cc.has_argument('-D_FORTIFY_SOURCE=2')
  c_hard += ['-D_FORTIFY_SOURCE=2']
endif

if cc.has_argument('-fno-plt')
  c_hard += ['-fno-plt']
endif

if cc.has_argument('-fstack-clash-protection')
  c_hard += ['-fstack-clash-protection']
endif

add_project_arguments(c_hard, language: 'c')

# ---- Hardening (link) ----
ld_hard = []

if host in ['linux', 'freebsd', 'netbsd']
  if cc.has_link_argument('-Wl,-z,relro')
    ld_hard += ['-Wl,-z,relro']
  endif
  if cc.has_link_argument('-Wl,-z,now')
    ld_hard += ['-Wl,-z,now']
  endif
  # Fail on unresolved at link time (ELF)
  if not with_san
    if cc.has_link_argument('-Wl,-z,defs')
      ld_hard += ['-Wl,-z,defs']   # only when NOT using sanitizers
    endif
  endif
  # Avoid linking unused libs
  if cc.has_link_argument('-Wl,--as-needed')
    ld_hard += ['-Wl,--as-needed']
  endif
endif

add_project_link_arguments(ld_hard, language: 'c')

# ---- Tests ----
tests_bin = executable('cargs-tests', ['tests/cargs.tests.c'],
  include_directories: include_directories('src/'))

test('cargs-tests', tests_bin, suite: ['unit'])

# ---- Examples ----
if get_option('examples').enabled()
  subdir('examples')
endif

# ---- Install ----
# Install the single header under include/cargs/cargs.h
install_headers('src/c-args-parser.h', subdir: 'c-args-parser')

pkg.generate(
  name: 'c-args-parser',
  description: 'Header-only, zero-alloc, callback CLI parser for C',
  version: meson.project_version(),
  subdirs: ['c-args-parser'],
  filebase: 'c-args-parser'
)
