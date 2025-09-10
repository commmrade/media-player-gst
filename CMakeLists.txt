cmake_minimum_required(VERSION 3.10.0)
project(proj VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)

# Базовый GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_executable(proj main.c utils.c state.c)

# Инклуды
target_include_directories(proj PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
)

# Линки
target_link_libraries(proj PRIVATE
    ${GSTREAMER_LIBRARIES}
)

# Флаги компилятора (например, -pthread)
target_compile_options(proj PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
)
