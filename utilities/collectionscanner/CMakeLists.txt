
########### next target ###############

qt4_automoc( CollectionScanner.cpp )

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
)

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS} ${libchardet_SRCS})

target_link_libraries(amarokcollectionscanner 
    Qt5::Core
    amarokshared
)

if(APPLE)
    set_target_properties(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/amarok.app/Contents/MacOS )
else()
    install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif()
