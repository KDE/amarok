add_definitions(${KDE4_ENABLE_EXCEPTIONS})
 
set(amarokqtjson_LIB_SRCS
    JsonQt/lib/JsonToProperties.cpp
    JsonQt/lib/JsonToVariant.cpp
    JsonQt/lib/ParseException.cpp
    JsonQt/lib/VariantToJson.cpp
)

QT4_AUTOMOC(
    JsonQt/lib/JsonToProperties.cpp
    JsonQt/lib/JsonToVariant.cpp
    JsonQt/lib/ParseException.cpp
    JsonQt/lib/VariantToJson.cpp
)

ADD_LIBRARY(amarokqtjson SHARED ${amarokqtjson_LIB_SRCS})
target_link_libraries(amarokqtjson ${QT_QTCORE_LIBRARY})

set_target_properties(amarokqtjson PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS amarokqtjson ${INSTALL_TARGETS_DEFAULT_ARGS} )