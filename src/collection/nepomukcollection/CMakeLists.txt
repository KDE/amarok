# not sure if that is good idea. but as Nepomuk is part of kdelibs i think
# require it shouldn't be a problem

find_package(Nepomuk REQUIRED)

include_directories( ../..
        ../../plugin
   ../../meta
   ../../collection
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} 
   ${NEPOMUK_INCLUDES})


########### next target ###############

set(amarok_collection-nepomukcollection_PART_SRCS
   NepomukCollection.cpp
   NepomukQueryMaker.cpp)


kde4_add_plugin(amarok_collection-nepomukcollection WITH_PREFIX ${amarok_collection-nepomukcollection_PART_SRCS})

target_link_libraries(amarok_collection-nepomukcollection 
    amaroklib 
    ${KDE4_THREADWEAVER_LIBRARIES} 
    ${NEPOMUK_LIBRARIES})

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-nepomukcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-nepomukcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-nepomukcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
