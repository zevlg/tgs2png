cmake_minimum_required(VERSION 3.3)

project(tgs2png VERSION 0.0.1 LANGUAGES C CXX)


# binary
add_executable(tgs2png "tgs2png.cpp")
target_compile_options(tgs2png PRIVATE -std=c++11)
target_link_libraries(tgs2png rlottie png zlib)
