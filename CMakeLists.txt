cmake_minimum_required(VERSION 3.3)

project(tgs2png VERSION 0.1.0 LANGUAGES C)

add_compile_options(-Wall -Werror)

find_package(PkgConfig REQUIRED)

pkg_check_modules(RLOTTIE REQUIRED rlottie)
pkg_check_modules(PNG REQUIRED libpng)

option (USE_ASAN "build all executables with asan" OFF)
if (USE_ASAN)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan")
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
endif (USE_ASAN)

# binary
add_executable(tgs2png "tgs2png.c")
target_compile_options(tgs2png PRIVATE)
target_link_libraries(tgs2png ${RLOTTIE_LIBRARIES} ${PNG_LIBRARIES} m)
