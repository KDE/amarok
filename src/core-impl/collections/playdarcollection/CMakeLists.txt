set( amarok_collection-playdarcollection_PART_SRCS
     PlaydarMeta.cpp
     PlaydarCollection.cpp
     PlaydarQueryMaker.cpp
     support/Controller.cpp
     support/Query.cpp
     support/ProxyResolver.cpp
     support/QMFunctionTypes.h )

add_library(amarok_collection-playdarcollection MODULE ${amarok_collection-playdarcollection_PART_SRCS})

target_link_libraries( amarok_collection-playdarcollection
                       amarokcore
                       amaroklib
                       KF5::ThreadWeaver
                       KF5::KIOCore )

install(TARGETS amarok_collection-playdarcollection DESTINATION ${KDE_INSTALL_PLUGINDIR} )
