## Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
##
## Redistribution and use is allowed according to the terms of the GNU license.
##
## Including and linking of all the tagguessing classes starts from here.
## A new library "tagguessing_musicbrainz" is created here
##
## Macros related to external libraries (like LIBCHROMAPRINT_FOUND) should be added in amarok/cmake/modules/

#include_directories(
#                    ${KDE4_INCLUDE_DIR}
#                    ${QT_INCLUDES}
#                    ${Amarok_SOURCE_DIR}/src
#                   )
#kde4_add_library( tagguessing_musicbrainz SHARED ${tagguessing_musicbrainz_SRCS} )

#target_link_libraries( tagguessing_musicbrainz
#    tagguessing_support
#    amarokcore
#    amaroklib
#    ${KDE4_KDECORE_LIBS}
#    ${KDE4_KDEUI_LIBS}
#    ${KDE4_THREADWEAVER_LIBRARIES}
#    ${QT_QTNETWORK_LIBRARY}
#    ${QT_QTXML_LIBRARY}
#)
