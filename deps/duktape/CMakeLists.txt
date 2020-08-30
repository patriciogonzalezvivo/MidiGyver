project(duktape)
add_definitions(-DDUK_OPT_CPP_EXCEPTIONS)
set_source_files_properties(duktape.c PROPERTIES LANGUAGE CXX)
add_library(duktape STATIC duktape.c duktape.h duk_config.h)

