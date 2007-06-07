
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. 
                    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_definitions(${TAGLIB_CFLAGS})

########### next target ###############

set(amarokcollectionscanner_SRCS main.cpp collectionscanner.cpp)

set(amarok_collection_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../amarokcore/org.kde.amarok.collection.xml)
QT4_ADD_DBUS_INTERFACE( amarokcollectionscanner_SRCS ${amarok_collection_xml} amarok_collection_interface )


kde4_automoc(${amarokcollectionscanner_SRCS})

kde4_add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS})

target_link_libraries(amarokcollectionscanner  ${KDE4_KDECORE_LIBS} amarok_taglib amarok ${KDE4_KDE3SUPPORT_LIBS} ${QT_QT3SUPPORT_LIBRARY} )

install(TARGETS amarokcollectionscanner DESTINATION ${BIN_INSTALL_DIR} )

