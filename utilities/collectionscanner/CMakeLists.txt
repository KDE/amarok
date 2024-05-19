
########### next target ###############

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
)

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS})

target_link_libraries(amarokcollectionscanner
    Qt5::Core
    amarokshared
)

if(APPLE)
    set_target_properties(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarokcollectionscanner DESTINATION ${KDE_INSTALL_BUNDLEDIR}/amarok.app/Contents/MacOS )
else()
    install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${KDE_INSTALL_BINDIR} )
endif()
