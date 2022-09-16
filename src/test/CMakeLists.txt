file(GLOB test_sources CONFIGURE_DEPENDS "*.c")

add_library(test STATIC ${test_sources})
