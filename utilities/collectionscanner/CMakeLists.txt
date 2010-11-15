
include_directories( ../
                     ${CMAKE_SOURCE_DIR}
                     ${QT_INCLUDES}
                     ${KDE4_KDECORE_LIBS}
                     ${TAGLIB_INCLUDES}
                     ${TAGLIB-EXTRAS_INCLUDES}
                   )

add_definitions(${TAGLIB_CFLAGS} ${TAGLIB-EXTRAS_CFLAGS})

#####################################################################
# TagLib File Type Resolvers
#####################################################################
set( taglib_filetype_resolver_SRCS
    ../../shared/taglib_filetype_resolvers/wavfiletyperesolver.cpp
    ../../shared/taglib_filetype_resolvers/mp4filetyperesolver.cpp
    ../../shared/taglib_filetype_resolvers/asffiletyperesolver.cpp
)

########### next target ###############

if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif(CMAKE_COMPILER_IS_GNUCXX)

qt4_automoc( CollectionScanner.cpp )

add_definitions( "-DUTILITIES_BUILD" )

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
    Directory.cpp
    Album.cpp
    Track.cpp
    Playlist.cpp
    BatchFile.cpp
    ../../shared/MetaTagLib.cpp
    ../../shared/MetaReplayGain.cpp
    ../../shared/FileType.cpp
)

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS} ${libchardet_SRCS} ${taglib_filetype_resolver_SRCS})

target_link_libraries(amarokcollectionscanner 
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
                      ${KDE4_KDECORE_LIBRARY}
                      ${TAGLIB_LIBRARIES}
                      ${TAGLIB-EXTRAS_LIBRARIES}
                      )

if(APPLE)
	SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	# install to app bundle on os x, otherwise amarok fails to load it
	install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
	install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
