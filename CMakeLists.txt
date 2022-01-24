cmake_minimum_required(VERSION 3.3)

project(tgs2png VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(RLOTTIE rlottie)

if (NOT RLOTTIE_FOUND)
  add_subdirectory("rlottie")

  set(RLOTTIE_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/rlottie/inc")
  set(RLOTTIE_LIBRARY_DIRS "${PROJECT_BUILD_DIR}/rlottie")
  set(RLOTTIE_LIBRARIES "rlottie")
endif ()

option (USE_ASAN "build all executables with asan" OFF)
if (USE_ASAN)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan")
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
endif (USE_ASAN)

# binary
add_compile_options(-Wall -Werror)
add_executable(tgs2png "tgs2png.c")

target_include_directories(tgs2png PUBLIC ${PNG_INCLUDE_DIRS})
target_link_directories(tgs2png PRIVATE ${PNG_LIBRARY_DIRS})

target_compile_options(tgs2png PRIVATE)
target_link_libraries(tgs2png ${RLOTTIE_LIBRARIES} ${PNG_LIBRARIES} m)

install(TARGETS tgs2png DESTINATION ${CMAKE_INSTALL_BINDIR})
