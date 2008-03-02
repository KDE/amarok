
include_directories( ../
                     ../meta
                    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_definitions(${TAGLIB_CFLAGS})

########### next target ###############

set(amarokcollectionscanner_SRCS main.cpp collectionscanner.cpp)

set(amarok_collection_xml  ../amarokcore/org.kde.amarok.collection.xml)
QT4_ADD_DBUS_INTERFACE( amarokcollectionscanner_SRCS ${amarok_collection_xml} amarok_collection_interface )

kde4_add_executable(amarokcollectionscanner NOGUI ${amarokcollectionscanner_SRCS})

target_link_libraries(amarokcollectionscanner amarok_taglib amaroklib )

install(TARGETS amarokcollectionscanner DESTINATION ${BIN_INSTALL_DIR} )

