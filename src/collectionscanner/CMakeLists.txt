
include_directories( ../
                     ../meta
                    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_definitions(${TAGLIB_CFLAGS})

########### next target ###############

set(amarokcollectionscanner_SRCS main.cpp collectionscanner.cpp)

set(amarok_collection_xml  ../dbus/org.kde.amarok.collection.xml)
QT4_ADD_DBUS_INTERFACE( amarokcollectionscanner_SRCS ${amarok_collection_xml} amarok_collection_interface )

kde4_add_executable(amarokcollectionscanner NOGUI ${amarokcollectionscanner_SRCS})

target_link_libraries(amarokcollectionscanner amarok_taglib ${KDE4_KDEUI_LIBS} )

if(APPLE)
	SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarokcollectionscanner ${INSTALL_TARGETS_DEFAULT_ARGS} )

