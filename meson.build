project('dosemu2-gui', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11',
    'b_pch=false'  # Required by libui
  ]
)

# Dependencies
libui_dep = dependency('libui', fallback : ['libui', 'libui_dep'])
subprocess_dep = dependency('subprocess', fallback : ['subprocess', 'subprocess_dep'])

# Include directories
incdir = include_directories('include')

# Source files
src_files = [
  'src/main.c',
  'src/ui_main.c',
  'src/dosemu_integration.c',
]

executable('dosemu2-gui',
  src_files,
  include_directories : incdir,
  dependencies : [libui_dep, subprocess_dep],
  install : true,
)
