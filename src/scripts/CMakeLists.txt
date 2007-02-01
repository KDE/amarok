
add_subdirectory( amarok_live )
add_subdirectory( common )
add_subdirectory( lyrics_astraweb )
add_subdirectory( lyrics_lyrc )
add_subdirectory( playlist2html )
add_subdirectory( ruby_debug )
add_subdirectory( score_default )
add_subdirectory( score_impulsive )
add_subdirectory( templates )
add_subdirectory( webcontrol )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = \
#    amarok_live \
#    common \
#    lyrics_astraweb \
#    lyrics_lyrc \
#    playlist2html \
#    ruby_debug \
#    score_default \
#    score_impulsive \
#    templates \
#    webcontrol
