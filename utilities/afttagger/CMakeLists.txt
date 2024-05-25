include_directories( ../
)
include_directories(SYSTEM
                     ${TAGLIB_INCLUDES}
                   )

add_definitions(${TAGLIB_CFLAGS})

set(amarok_afttagger_SRCS
    SafeFileSaver.cpp
    AFTTagger.cpp
)

add_executable(amarok_afttagger ${amarok_afttagger_SRCS} )

if( TAGLIB-EXTRAS_FOUND )
    include_directories(SYSTEM ${TAGLIB-EXTRAS_INCLUDES})
    add_definitions(${TAGLIB-EXTRAS_CFLAGS})
    target_link_libraries(amarok_afttagger ${TAGLIB-EXTRAS_LIBRARIES})
endif()

target_link_libraries(amarok_afttagger
                        Qt::Core
                        ${TAGLIB_LIBRARIES}
                     )


if(APPLE)
    set_target_properties(amarok_afttagger PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarok_afttagger DESTINATION ${KDE_INSTALL_BUNDLEDIR}/Amarok.app/Contents/MacOS )
else()
    install(TARGETS amarok_afttagger RUNTIME DESTINATION ${KDE_INSTALL_BINDIR} )
endif()
