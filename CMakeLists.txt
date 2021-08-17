cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_DIR})
    set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
endif()

add_definitions(/W4)
PROJECT(Render)

SET(BINARIES_RELATIVE_PATH /bin/)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}${BINARIES_RELATIVE_PATH})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}${BINARIES_RELATIVE_PATH})

file(GLOB SourceFiles "src/*.cpp")
file(GLOB HeaderFiles "src/*.h")
source_group("Header Files" FILES ${HeaderFiles}) 
source_group("Source Files" FILES ${SourceFiles}) 


file(GLOB GLSLs "shader/*.glsl")
source_group("Shader Files" FILES ${GLSLs}) 

find_package(yaml-cpp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
add_executable(renderBoat ${HeaderFiles} ${SourceFiles} ${GLSLs})

set_property(TARGET renderBoat PROPERTY CXX_STANDARD 17)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(renderBoat PRIVATE ${STB_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(renderBoat PRIVATE "./src")
TARGET_INCLUDE_DIRECTORIES(renderBoat PRIVATE ${OPENGL_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(renderBoat ${OPENGL_LIBRARIES})
target_link_libraries(renderBoat  Eigen3::Eigen)
TARGET_LINK_LIBRARIES(renderBoat GLEW::GLEW)
TARGET_LINK_LIBRARIES(renderBoat glm::glm)
TARGET_LINK_LIBRARIES(renderBoat GLUT::GLUT)
target_link_libraries(renderBoat assimp::assimp)
target_link_libraries(renderBoat yaml-cpp)
