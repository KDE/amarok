

########### next target ###############

set(amarok_bin_SRCS loader.cpp splash.cpp )

kde4_automoc(${amarok_bin_SRCS})

kde4_add_executable(amarok_bin ${amarok_bin_SRCS})

set_target_properties(amarok_bin PROPERTIES OUTPUT_NAME amarok)

target_link_libraries(amarok_bin  ${KDE4_KDECORE_LIBS} )

install_targets(/bin amarok_bin )


########### install files ###############




#original Makefile.am contents follow:

#bin_PROGRAMS = amarok
#
#INCLUDES = \
#        $(all_includes)
#
#amarok_SOURCES = \
#        loader.cpp \
#        splash.cpp
#
#amarok_LDADD = \
#        $(LIB_QT) \
#        $(LIB_KDECORE)
#
#amarok_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#METASOURCES = AUTO
#
