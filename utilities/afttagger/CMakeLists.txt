include_directories( ../
                     ${QT_INCLUDES}             
                     ${TAGLIB_INCLUDES}         
                   )                            

if ( TAGLIB-EXTRAS_FOUND )
    include_directories( ${include_directories} ${TAGLIB-EXTRAS_INCLUDE_DIR} )
endif ( TAGLIB-EXTRAS_FOUND )                                                 

add_definitions(${TAGLIB_CFLAGS})

set(amarok_afttagger_SRCS
    SafeFileSaver.cpp
    AFTTagger.cpp
)

add_executable(amarok_afttagger ${amarok_afttagger_SRCS} )

target_link_libraries(amarok_afttagger
                        ${QT_QTCORE_LIBRARY}
                        ${TAGLIB_LIBRARIES}
                     )


if(APPLE)
    SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
    install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)

