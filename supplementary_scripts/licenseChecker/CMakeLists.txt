cmake_minimum_required(VERSION 2.6)
project(AmarokLicenseChecker)

find_package(Qt4 REQUIRED)

set(CMAKE_CXX_FLAGS "-g -Wall")

include(${QT_USE_FILE})

set (checker_SRCS
	main.cpp
	log.cpp
)

add_executable(amaroklicensechecker ${checker_SRCS})
target_link_libraries(amaroklicensechecker ${QT_LIBRARIES})

install(TARGETS amaroklicensechecker DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

