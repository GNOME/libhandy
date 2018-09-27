glade_catalogdir = join_paths(get_option('prefix'), get_option('datadir'), 'glade', 'catalogs')
glade_modulesdir = join_paths(get_option('prefix'), get_option('libdir'), 'glade', 'modules')

install_data('libhandy.xml', install_dir: glade_catalogdir)

glade_handy_deps = [
  dependency('gladeui-2.0'),
  libhandy_dep,
]

glade_handy_sources = [
  'glade-hdy-header-group.c',
]

glade_handy_args = [
  '-DG_LOG_DOMAIN="GladeUI-Handy"',
]

glade_handy = shared_library(
  'gladehandy-' + apiversion,
  glade_handy_sources,
  soversion: 0,
  c_args: glade_handy_args,
  dependencies: glade_handy_deps,
  include_directories: [ root_inc, src_inc ],
  install: true,
  install_dir: glade_modulesdir,
)
