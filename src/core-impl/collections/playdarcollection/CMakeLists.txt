add_definitions(${KDE4_ENABLE_EXCEPTIONS})

include_directories( ${Amarok_SRC_DIR}/src
                     ${KDE4_INCLUDE_DIR}
                     ${QT_INCLUDES} )

set( amarok_collection-playdarcollection_PART_SRCS
     PlaydarMeta.cpp
     PlaydarCollection.cpp
     PlaydarQueryMaker.cpp
     support/Controller.cpp
     support/Query.cpp
     support/ProxyResolver.cpp
     support/QMFunctionTypes.h )

kde4_add_plugin(amarok_collection-playdarcollection ${amarok_collection-playdarcollection_PART_SRCS})

target_link_libraries( amarok_collection-playdarcollection
                       amarokcore
                       amaroklib
                       amarokqtjson
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_THREADWEAVER_LIBRARIES}
                       ${KDE4_KIO_LIBS} )

install(TARGETS amarok_collection-playdarcollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES amarok_collection-playdarcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
