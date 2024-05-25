project(Amarok-Update-Signer)

cmake_minimum_required(VERSION 2.6.2)

find_package(Qt4 REQUIRED)
find_package(KDE4 REQUIRED) #Needed for the QCA2 check to work
find_package(QCA2 REQUIRED)

# we need the QtCrypto library for the signer
if( QCA2_FOUND )
    include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/shared
    )
    include_directories(SYSTEM
        ${QCA2_INCLUDE_DIR}
    )

    set(amarok-update-signer_SRCS signer.cpp amarok-update-signer.cpp)
    qt4_automoc(${amarok-update-signer_SRCS})
    add_executable(amarok-update-signer ${amarok-update-signer_SRCS})
    target_link_libraries(amarok-update-signer Qt::Core ${QCA2_LIBRARIES})

    if(APPLE)
        set_target_properties(amarok-update-signer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
        # install to app bundle on os x, otherwise amarok fails to load it
        install(TARGETS amarok-update-signer DESTINATION ${KDE_INSTALL_BUNDLEDIR}/Amarok.app/Contents/MacOS )
    else()
        install(TARGETS amarok-update-signer RUNTIME DESTINATION ${KDE_INSTALL_BINDIR} )
    endif()
endif()
