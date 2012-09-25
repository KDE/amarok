
include_directories( ../
                     ${CMAKE_SOURCE_DIR}
                     ${QT_INCLUDES}
                   )

########### next target ###############

if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif(CMAKE_COMPILER_IS_GNUCXX)

qt4_automoc( CollectionScanner.cpp )

add_definitions( "-DUTILITIES_BUILD" )

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
    ${CMAKE_SOURCE_DIR}/shared/FileType.cpp
    ${CMAKE_SOURCE_DIR}/shared/MetaTagLib.cpp
    ${CMAKE_SOURCE_DIR}/shared/TagsFromFileNameGuesser.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/Album.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/BatchFile.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/Directory.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/Playlist.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/ScanningState.cpp
    ${CMAKE_SOURCE_DIR}/shared/collectionscanner/Track.cpp
)
if( TAGLIB_FOUND )
    set(amarokcollectionscanner_SRCS
        ${amarokcollectionscanner_SRCS}
        ../../shared/FileTypeResolver.cpp
        ../../shared/MetaReplayGain.cpp
        ../../shared/tag_helpers/APETagHelper.cpp
        ../../shared/tag_helpers/ASFTagHelper.cpp
        ../../shared/tag_helpers/ID3v2TagHelper.cpp
        ../../shared/tag_helpers/MP4TagHelper.cpp
        ../../shared/tag_helpers/StringHelper.cpp
        ../../shared/tag_helpers/TagHelper.cpp
        ../../shared/tag_helpers/VorbisCommentTagHelper.cpp
    )
endif( TAGLIB_FOUND )

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS} ${libchardet_SRCS})

if( TAGLIB_FOUND )
    include_directories(${TAGLIB_INCLUDES})
    add_definitions(${TAGLIB_CFLAGS})
    target_link_libraries(amarokcollectionscanner ${TAGLIB_LIBRARIES})
endif( TAGLIB_FOUND )
if( TAGLIB-EXTRAS_FOUND )
    include_directories(${TAGLIB-EXTRAS_INCLUDES})
    add_definitions(${TAGLIB-EXTRAS_CFLAGS})
    target_link_libraries(amarokcollectionscanner ${TAGLIB-EXTRAS_LIBRARIES})
endif( TAGLIB-EXTRAS_FOUND )

target_link_libraries(amarokcollectionscanner 
                      ${QT_QTGUI_LIBRARY} # required on Solaris, see bug 264112
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
                      ${KDE4_KDECORE_LIBRARY}
                      )

if(APPLE)
	SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	# install to app bundle on os x, otherwise amarok fails to load it
	install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
	install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
