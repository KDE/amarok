
include_directories( ../..
   ../../collection
   ../../dialogs
#./search-api
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} 
   ${NEPOMUK_INCLUDES}
   ${SOPRANO_INCLUDE_DIR})


########### next target ###############

set(amarok_collection-nepomukcollection_PART_SRCS
   NepomukAlbum.cpp
   NepomukCollection.cpp
   NepomukComposer.cpp
   NepomukGenre.cpp
   NepomukQueryMaker.cpp
   NepomukRegistry.cpp
   NepomukArtist.cpp
   NepomukTrack.cpp
   NepomukYear.cpp)
#./search-api/query.cpp
#   ./search-api/queryparser.cpp
#   ./search-api/searchcore.cpp
#   ./search-api/searchthread.cpp
#  ./search-api/term.cpp
#   ./search-api/searchresult.cpp


kde4_add_plugin(amarok_collection-nepomukcollection ${amarok_collection-nepomukcollection_PART_SRCS})

target_link_libraries(amarok_collection-nepomukcollection 
    amarokcore
    amaroklib
    amarokpud
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_KDEUI_LIBS}
    ${NEPOMUK_LIBRARIES}
    ${SOPRANO_LIBRARIES})

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-nepomukcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-nepomukcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-nepomukcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
