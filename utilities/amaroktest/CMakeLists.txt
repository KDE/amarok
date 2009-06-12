
include_directories( ../
                     ${QT_INCLUDES}
                     ${CMAKE_CURRENT_BINARY_DIR}
                   )

########### next target ###############

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

set(amaroktest_SRCS AmarokTest.cpp)

qt4_automoc( ${amaroktest_SRCS} )

add_executable(amaroktest ${amaroktest_SRCS})

target_link_libraries(amaroktest
                      amaroklib
                     )

if(APPLE)
	SET_TARGET_PROPERTIES(amaroktest PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	# install to app bundle on os x, otherwise amarok fails to load it
	install(TARGETS amaroktest DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
	install(TARGETS amaroktest RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
