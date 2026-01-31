project('infinity-plugin', ['c', 'cpp'],
  version: '0.9.0-dev',
  meson_version: '>=1.1.0',
  default_options: [
    'c_std=gnu99',
    'cpp_std=c++17',
    'warning_level=2',
  ],
)

fs = import('fs')

glib_dep = dependency('glib-2.0', version: '>=2.28')
audacious_dep = dependency('audacious', version: '>=3.6')
qt_dep = dependency('qt6', modules: ['Widgets'])

cc = meson.get_compiler('c')

if host_machine.system() == 'windows'
  export_define = '__declspec(dllexport)'
elif cc.get_id() in ['gcc', 'clang']
  add_project_arguments('-fvisibility=hidden', language: ['c', 'cpp'])
  export_define = '__attribute__((visibility("default")))'
else
  error('Unknown syntax for EXPORT keyword')
endif

config_data = configuration_data()
config_data.set('EXPORT', export_define)
config_data.set('INFINITY_DEBUG', get_option('infinity_debug'))
config_data.set('HAVE_CONFIG_H', 1)
config_data.set_quoted('PACKAGE', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
configure_file(output: 'config.h', configuration: config_data)

if cc.get_id() == 'gcc'
  add_project_arguments(
    '-Wimplicit',
    '-Wunused',
    '-Wmissing-prototypes',
    language: 'c',
  )
  if get_option('vectorization')
    add_project_arguments('-ftree-vectorize', language: 'c')
  endif
endif

datadir = join_paths(get_option('prefix'), get_option('datadir'), 'infinity-plugin')
add_project_arguments(
  '-DDATADIR="@0@"'.format(datadir),
  language: ['c', 'cpp'],
)

prefix = get_option('prefix')
libdir = get_option('libdir')
if fs.exists(join_paths(prefix, 'lib', 'x86_64-linux-gnu', 'libaudcore.so'))
  plugin_install_dir = join_paths(libdir, 'x86_64-linux-gnu', 'audacious', 'Visualization')
else
  plugin_install_dir = join_paths(libdir, 'audacious', 'Visualization')
endif

subdir('minidocs')
subdir('src')
