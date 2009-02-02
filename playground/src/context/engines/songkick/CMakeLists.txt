include_directories( ../../../../../src/
                     ../../../../../src/context
                   )

add_definitions(${KDE4_ENABLE_EXCEPTIONS})

set(amarokqtjson_LIB_SRCS
    JsonQt/ParseException.cpp
    JsonQt/JsonToProperties.cpp
    JsonQt/JsonToVariant.cpp
    JsonQt/VariantToJson.cpp
)

ADD_LIBRARY(amarokqtjson SHARED ${amarokqtjson_LIB_SRCS})
target_link_libraries(amarokqtjson ${QT_QTCORE_LIBRARY})

set_target_properties(amarokqtjson PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS amarokqtjson ${INSTALL_TARGETS_DEFAULT_ARGS} )

############################################################################################

set( songkick_engine_SRCS
    SongkickEngine.cpp
)

kde4_add_plugin(amarok_data_engine_songkick ${songkick_engine_SRCS})
target_link_libraries( amarok_data_engine_songkick amaroklib ${KDE4_PLASMA_LIBS} amarokqtjson ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_songkick DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-songkick.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
