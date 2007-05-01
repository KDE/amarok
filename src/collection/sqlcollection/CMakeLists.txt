include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
	${CMAKE_CURRENT_SOURCE_DIR}/../../meta
	${CMAKE_CURRENT_SOURCE_DIR}/../../collection
        ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES} )


########### next target ###############

set(amarok_collection-sqlcollection_PART_SRCS
	sqlcollection.cpp
	sqlquerybuilder.cpp
	sqlregistry.cpp
	sqlmeta.cpp
	sqlitecollection.cpp
	mysqlcollection.cpp
	mysqlquerymaker.cpp
	postgresqlcollection.cpp
	postgresqlquerymaker.cpp )

kde4_automoc(${amarok_collection-sqlcollection_PART_SRCS})

kde4_add_plugin(amarok_collection-sqlcollection WITH_PREFIX ${amarok_collection-sqlcollection_PART_SRCS})

target_link_libraries(amarok_collection-sqlcollection amarok ${KDE4_KDECORE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES})

install(TARGETS amarok_collection-sqlcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-sqlcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
