cmake_minimum_required(VERSION 3.10)
project(raytrace)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE LEGACY)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(raytrace raytrace.cpp util.cpp)
target_link_libraries(raytrace PRIVATE glfw ${GLEW_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})
target_include_directories(raytrace PRIVATE ${GLEW_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})

add_executable(knn knn.cpp util.cpp gl.cpp)
target_link_libraries(knn PRIVATE glfw ${GLEW_LIBRARIES} OpenGL::GL OpenGL::EGL gbm)
target_include_directories(knn PRIVATE ${GLEW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
