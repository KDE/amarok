include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
	${CMAKE_CURRENT_SOURCE_DIR}/../../meta
	${CMAKE_CURRENT_SOURCE_DIR}/../../collection
	${AMAROK_COLLECTION_SUPPORT_DIR}
        ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES} )


########### next target ###############

set(amarok_collection-daapcollection_PART_SRCS
	daapmeta.cpp
#	daapcollection.cpp
	${AMAROK_COLLECTION_SUPPORT_DIR}/memoryquerymaker.cpp )

kde4_automoc(${amarok_collection-daapcollection_PART_SRCS})

kde4_add_plugin(amarok_collection-daapcollection WITH_PREFIX ${amarok_collection-daapcollection_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_collection-daapcollection )

target_link_libraries(amarok_collection-daapcollection amarok ${KDE4_KDECORE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})

#install(TARGETS amarok_collection-daapcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

#install( FILES amarok_collection-daapcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
