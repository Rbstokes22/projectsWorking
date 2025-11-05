# SRC CMAKE DIR

# --------------------------------------------------------------
# This file defines build targets that live under /src.
# It creates:
#   (1) A static library target for "asyncProj" (the core logic)
#   (2) A test executable "test_async" that links to that library.
# --------------------------------------------------------------

# Add the subdirectory containing the Async component.
# This allows Async to define itself as a library in its own CMakeLists.
# For more than 1 subdir, add them the same way as you would this, if they
# contain their own CMakeList.
add_subdirectory(Async)

# --------------------------------------------------------------
# Define the test executable
# --------------------------------------------------------------
# This will compile /src/main.cpp into an executable named "test_async".
# It is used for local testing and development of the async library.
add_executable(test_async main.cpp)

# --------------------------------------------------------------
# Link the executable against the asyncProj library (built in /src/Async).
# This allows main.cpp to use all public symbols from asyncProj.
# --------------------------------------------------------------
target_link_libraries(test_async
    PRIVATE
        asyncProj
)

# --------------------------------------------------------------
# Include directories for this target.
# Although the root CMakeLists already adds ${PROJECT_SOURCE_DIR}/include,
# we repeat it here for clarity and local scope.
# This ensures that both /include and any subfolders under it (e.g. Async/)
# are searched for headers when compiling this target.
# PRIVATE indicates that /include applies to this specific target.
# --------------------------------------------------------------
target_include_directories(test_async
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# --------------------------------------------------------------
# Optional: Add extra compiler warnings for safety and portability.
# These flags apply only to this target, not globally.
# --------------------------------------------------------------
target_compile_options(test_async
    PRIVATE
        -Wall -Wextra -pedantic
)
