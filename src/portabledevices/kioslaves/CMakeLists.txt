project(pmp_kioslave)

include_directories( ${KDE4_INCLUDE_DIR} ${KDE4_KIO_INCLUDES} ${QT_INCLUDES} )

if(MTP_FOUND)
    ADD_DEFINITIONS(-DHAVE_MTP)
    set( mtp_SRCS
        mtp_backend/pmpkioslave_mtpbackend.cpp
    )
    set( mtp_LIBS
        mtp
    )
else(MTP_FOUND)
    set( mtp_SRCS )
    set( mtp_LIBS )
endif(MTP_FOUND)


set(kio_pmp_PART_SRCS
    ${mtp_SRCS}
    pmpkioslave.cpp
    pmpbackend.cpp
)

kde4_add_plugin( kio_pmp ${kio_pmp_PART_SRCS} )

target_link_libraries( kio_pmp ${KDE4_KIO_LIBS} ${mtp_LIBS} )

install( TARGETS kio_pmp DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES
    pmp.protocol
DESTINATION ${SERVICES_INSTALL_DIR} )
