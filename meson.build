project('rtsp_server', 'cpp',
  version: '1.0',
  default_options: ['cpp_std=c++17']
)

# Find dependencies using pkg-config
gst_dep = dependency('gstreamer-1.0', required: true)
gst_rtsp_dep = dependency('gstreamer-rtsp-server-1.0', required: true)

executable(
  'rtsp_server',
  'rtsp_server.cpp',
  dependencies: [gst_dep, gst_rtsp_dep],
  install: false
)
