cmake_minimum_required( VERSION 3.20.0 )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR} )

add_compile_options( "/MP" )
add_link_options( "/INCREMENTAL:NO" "/SUBSYSTEM:WINDOWS" )

project( clipboard )
set( SOURCES
	src/clip.cpp
	src/main.cpp
)
find_package( OpenCV REQUIRED )

add_executable( ${PROJECT_NAME} ${SOURCES} )
target_include_directories( ${PROJECT_NAME} PRIVATE include )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} )
