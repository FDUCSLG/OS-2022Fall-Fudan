file(GLOB kernel_sources CONFIGURE_DEPENDS "*.c")

add_library(kernelx STATIC ${kernel_sources})
