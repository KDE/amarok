
find_package(Qt4 REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QCA2_INCLUDE_DIR})

set(amarok-update-signer_SRCS signer.cpp amarok-update-signer.cpp)
qt4_automoc(${amarok-update-signer_SRCS})
add_executable(amarok-update-signer ${amarok-update-signer_SRCS})
target_link_libraries(amarok-update-signer ${QT_QTCORE_LIBRARY} ${QCA2_LIBRARIES})

if(APPLE)
    SET_TARGET_PROPERTIES(amarok-update-signer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    # install to app bundle on os x, otherwise amarok fails to load it
    install(TARGETS amarok-update-signer DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
    install(TARGETS amarok-update-signer RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
