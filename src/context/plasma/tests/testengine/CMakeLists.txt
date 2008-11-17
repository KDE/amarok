project(plasma_testengine_dataengine)

find_package(KDE4 REQUIRED)
find_package(Plasma REQUIRED)

include(KDE4Defaults)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${KDE4_INCLUDES}
                    ${PLASMA_INCLUDE_DIR})

set(testengine_engine_SRCS
    testengine.cpp)

kde4_add_plugin(plasma_engine_testengine
                ${testengine_engine_SRCS})

target_link_libraries(plasma_engine_testengine
                      ${KDE4_KIO_LIBS}
                      plasma)

install(TARGETS plasma_engine_testengine DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-dataengine-testengine.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
