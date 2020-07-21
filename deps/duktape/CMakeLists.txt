# add_library(duktape duktape.c)

# target_compile_options(duktape PRIVATE
#   -fstrict-aliasing
#   -fomit-frame-pointer
#   -std=c99
#   -Wall)

# target_include_directories(duktape PUBLIC .)

project(duktape)
add_definitions(-DDUK_OPT_CPP_EXCEPTIONS)
set_source_files_properties(duktape.c PROPERTIES LANGUAGE CXX)
add_library(duktape duktape.c duktape.h duk_config.h)

