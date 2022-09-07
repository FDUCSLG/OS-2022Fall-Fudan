file(GLOB common_sources CONFIGURE_DEPENDS "*.c")

add_library(common STATIC ${common_sources})
