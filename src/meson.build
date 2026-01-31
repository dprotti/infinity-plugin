src_inc = include_directories('.', '..')

common_deps = [glib_dep]

libinfinity_sources = files(
  'infinity.c',
  'compute.c',
  'display.c',
  'effects.c',
)

libinfinity = static_library(
  'infinity',
  sources: libinfinity_sources,
  include_directories: [src_inc],
  dependencies: common_deps,
)

infinite_lib = shared_library(
  'infinite',
  sources: ['audacious.cc', 'ui_gtk.cc'],
  include_directories: [src_inc],
  link_with: libinfinity,
  dependencies: [audacious_dep, glib_dep, gtk_dep],
  install: true,
  install_dir: plugin_install_dir,
)

install_data('infinite_states', install_dir: datadir)
