cmake_minimum_required(VERSION 3.20)
project(EZEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW 가져오기 (FetchContent)
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

add_executable(Sandbox
  src/app/Sandbox.cpp
  src/engine/Platform/GlfwWindow.cpp
)

target_include_directories(Sandbox PRIVATE src)
target_link_libraries(Sandbox PRIVATE glfw)

# Windows에서 콘솔 창 안 뜨게 하려면 WIN32 옵션을 쓰지만,
# 오늘은 로그를 보기 위해 콘솔 유지.