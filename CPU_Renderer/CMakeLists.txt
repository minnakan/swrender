# CMakeList.txt : CMake project for CPU_Renderer, include source and define
# project specific logic here.
#
include(FetchContent)
FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-2.28.5
  GIT_SHALLOW    TRUE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  SDL2_ttf
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
  GIT_TAG        release-2.20.2
  GIT_SHALLOW    TRUE)
set(SDL2TTF_INSTALL  OFF CACHE BOOL "" FORCE)
set(SDL2TTF_VENDORED ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2_ttf)

# Add source to this project's executable.
add_executable (CPU_Renderer "src/CPU_Renderer.cpp" "src/tgaimage.cpp" "src/model.cpp" "src/rasterizer.cpp" "src/camera.cpp" "src/shader.cpp" "src/pipeline.cpp")
target_include_directories(CPU_Renderer PRIVATE include)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(CPU_Renderer PRIVATE OpenMP::OpenMP_CXX)
endif()
target_link_libraries(CPU_Renderer PRIVATE SDL2::SDL2-static SDL2_ttf)
set_target_properties(CPU_Renderer PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_options(CPU_Renderer PRIVATE /ENTRY:mainCRTStartup)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CPU_Renderer PROPERTY CXX_STANDARD 20)
endif()

add_custom_command(TARGET CPU_Renderer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:CPU_Renderer>/resources)

# TODO: Add tests and install targets if needed.
