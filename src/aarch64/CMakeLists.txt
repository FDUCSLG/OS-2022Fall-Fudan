file(GLOB aarch64_sources CONFIGURE_DEPENDS "*.[Sc]")

add_library(aarch64 STATIC ${aarch64_sources})
