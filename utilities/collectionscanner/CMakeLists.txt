include_directories( ${QT_INCLUDES} )

########### next target ###############

qt4_automoc( CollectionScanner.cpp )

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
)

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS} ${libchardet_SRCS})

target_link_libraries(amarokcollectionscanner 
    ${QT_QTCORE_LIBRARY}
    amarokshared
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/amarok.app/Contents/MacOS )
else(APPLE)
    install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
