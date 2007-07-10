project(mtp_kioslave)

include_directories( ${KDE4_INCLUDE_DIR} ${KDE4_KIO_INCLUDES} ${QT_INCLUDES} )

#include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../klegendary
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../engine
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../mediadevice
#    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


set(kio_mtp_PART_SRCS
    mtp_kioslave.cpp
)

kde4_add_plugin(kio_mtp ${kio_mtp_PART_SRCS} )

target_link_libraries(kio_mtp ${KDE4_KIO_LIBS} mtp )

install( TARGETS kio_mtp DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES
    mtp.protocol
DESTINATION ${SERVICES_INSTALL_DIR} )
