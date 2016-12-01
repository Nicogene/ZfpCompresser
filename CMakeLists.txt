cmake_minimum_required(VERSION 3.2.2)

SET(PROJECTNAME zfpcompresser)
PROJECT(${PROJECTNAME})

# Make CMake aware of the cmake folder for local FindXXX scripts,
# append rather than set in case the user has passed their own
# additional paths via -D.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

message(${CMAKE_MODULE_PATH})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set postfixes for generated libraries based on buildtype.
set(CMAKE_RELEASE_POSTFIX "")
set(CMAKE_DEBUG_POSTFIX "-debug")

# Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

# Options
option(YARP "Enable use of YARP." ON)
option(ZFP "Enable use of ZFP compression library." ON)
#ZFP
if(ZFP)
	find_package(ZFP REQUIRED)
	if (ZFP_FOUND)
	message("-- Found YARP library: ${ZFP_LIBRARY}")
    	include_directories(${ZFP_INCLUDE_DIR})
	else (ZFP_FOUND)
    	message("-- Did not find ZFP library.")
	endif (ZFP_FOUND)

endif(ZFP)

# YARP.
if (YARP)
	find_package( YARP REQUIRED )
	if (YARP_FOUND)
		message("-- Found YARP library: ${YARP_LIBRARIES}")
    	include_directories(${YARP_INCLUDE_DIRS})
	else (YARP_FOUND)
    	message("-- Did not find YARP library.")
	endif (YARP_FOUND)
endif (YARP)


set(SOURCE_DIR ./src)
set(INCLUDE_DIR ./include)
include_directories(${SOURCE_DIR} ${INCLUDE_DIR} ../src ../include)
file(GLOB Executable_SOURCES ${SOURCE_DIR}/*.cpp)
file(GLOB Executable_HEADERS ${INCLUDE_DIR}/*.h)
add_executable(${PROJECTNAME} ${Executable_SOURCES} ${Executable_HEADERS})

target_link_libraries(${PROJECTNAME} ${YARP_LIBRARIES} ${ZFP_LIBRARY} )

if(${CMAKE_VERSION} VERSION_LESS 3.1)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
  check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
else()
  target_compile_features(${PROJECTNAME} PRIVATE cxx_range_for)
endif()

