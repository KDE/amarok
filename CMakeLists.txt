
OPTION(AMAROK_AMAZON_SUPPORT   "Enable Amazon cover download support")

add_subdirectory( src )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = \
#    src
#
#EXTRA_DIST = \
#    AUTHORS \
#    COPYING \
#    ChangeLog \
#    INSTALL \
#    README \
#    TODO \
#    admin \
#    amarok.kdevprj \
#    amarok.lsm
#
#AUTOMAKE_OPTIONS = \
#    foreign
#
