file(GLOB fs_sources CONFIGURE_DEPENDS "*.c")

add_library(fs STATIC ${fs_sources})
