project('infinity', ['c', 'cpp'],
  version: '2026.6.14',
  meson_version: '>=0.60.0',
  default_options: [
    'c_std=gnu99',
    'cpp_std=c++17',
    'warning_level=3',
    'buildtype=release',
  ],
)

fs = import('fs')

glib_dep    = dependency('glib-2.0', version: '>=2.28')
gtk_dep     = dependency('gtk+-3.0')
pipewire_dep = dependency('libpipewire-0.3', required: get_option('capture-backend') == 'pipewire')
pulse_dep    = dependency('libpulse',        required: get_option('capture-backend') == 'pulseaudio')

cc = meson.get_compiler('c')

if cc.get_id() in ['gcc', 'clang']
  add_project_arguments('-fvisibility=hidden', language: ['c', 'cpp'])
endif

config_data = configuration_data()
config_data.set('HAVE_CONFIG_H', 1)
config_data.set_quoted('PACKAGE', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set('CAPTURE_BACKEND_PIPEWIRE',   get_option('capture-backend') == 'pipewire'   ? 1 : 0)
config_data.set('CAPTURE_BACKEND_PULSEAUDIO', get_option('capture-backend') == 'pulseaudio' ? 1 : 0)
configure_file(output: 'config.h', configuration: config_data)

if cc.get_id() == 'gcc'
  add_project_arguments(
    '-Wimplicit',
    '-Wunused',
    '-Wmissing-prototypes',
    language: 'c',
  )
  if get_option('vectorization')
    add_project_arguments('-ftree-vectorize', language: ['c', 'cpp'])
  endif
endif

datadir = join_paths(get_option('prefix'), get_option('datadir'), 'infinity')
add_project_arguments(
  '-DDATADIR="@0@"'.format(datadir),
  language: ['c', 'cpp'],
)

subdir('minidocs')
subdir('src')
