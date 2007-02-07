
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarokcollectionscanner_SRCS main.cpp collectionscanner.cpp )

qt4_add_dbus_adaptor( amarokcollectionscanner_SRCS org.kde.amarok.scanner.xml collectionscannerdcophandler.h DbusCollectionScannerHandler )

kde4_automoc(${amarokcollectionscanner_SRCS})

kde4_add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS})

target_link_libraries(amarokcollectionscanner  ${KDE4_KDECORE_LIBS} metadata amarok )

install(TARGETS amarokcollectionscanner DESTINATION ${BIN_INSTALL_DIR} )

