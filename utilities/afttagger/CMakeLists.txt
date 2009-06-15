include_directories( ../
                     ${QT_INCLUDES}             
                     ${TAGLIB_INCLUDES}         
                     #${CMAKE_CURRENT_BINARY_DIR}
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


install(TARGETS amarok_afttagger ${INSTALL_TARGETS_DEFAULT_ARGS})

