# ASYNC CMAKE DIR

# --------------------------------------------------------------
# This file defines the Async module as a static library.
# The library will be named "asyncProj" to match the project name
# defined in the root CMakeLists.txt.
# --------------------------------------------------------------

# Create a static library target named "asyncProj" using async.cpp.
# STATIC means the compiled object code (.a) will be linked into
# executables at build time rather than at runtime (as with .so or .dll).
# With multiple source files list them all after STATIC.
add_library(asyncProj STATIC async.cpp)

# --------------------------------------------------------------
# Specify include directories for this target.
# PUBLIC means:
#   - asyncProj itself can use these headers
#   - Any target that links to asyncProj can also use these headers
# This allows other code (like main.cpp or future projects) to
# #include "Async/async.hpp" without extra configuration.
# --------------------------------------------------------------
target_include_directories(asyncProj
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# --------------------------------------------------------------
# (Optional) Add stricter compiler warnings just for this module.
# This helps keep library code portable and clean.
# --------------------------------------------------------------
target_compile_options(asyncProj
    PRIVATE
        -Wall -Wextra -Wpedantic
)
