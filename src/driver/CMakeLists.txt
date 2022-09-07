file(GLOB driver_sources CONFIGURE_DEPENDS "*.c")

add_library(driver STATIC ${driver_sources})
