src_inc = include_directories('.', '..')

capture_backend = get_option('capture-backend')

if capture_backend == 'pipewire'
  capture_dep = pipewire_dep
  capture_src = 'pipewire_capture.cc'
else
  capture_dep = pulse_dep
  capture_src = 'pulseaudio_capture.cc'
endif

common_deps = [glib_dep, gtk_dep, capture_dep]

libinfinity_sources = files(
  'infinity.cc',
  'compute.cc',
  'display.cc',
  'effects.c',
  capture_src,
)

libinfinity = static_library(
  'infinity',
  sources: libinfinity_sources,
  include_directories: [src_inc],
  dependencies: common_deps,
)

executable('infinity',
  sources: ['main.cc', 'ui_gtk.cc'],
  include_directories: [src_inc],
  link_with: libinfinity,
  dependencies: common_deps,
  install: true,
  install_dir: get_option('bindir'),
)

install_data('infinite_states', install_dir: datadir)
