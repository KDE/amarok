include_directories(
    ${LIBCHROMAPRINT_INCLUDE_DIR}
)

########### next target ###############

set( tagguessing_acustid_SRCS
    Acustidprovider.cpp
    Fingerprintcalculator.cpp
    Webservicequeryer.cpp
)

kde4_add_library( tagguessing_acustid SHARED ${tagguessing_acustid_SRCS} )

target_link_libraries( tagguessing_acustid
    amarokcore
    ${KDE4_KDECORE_LIBS}
)
