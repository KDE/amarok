amarokrubylibdir = $(kde_datadir)/amarok/ruby_lib

amarokrubylib_LTLIBRARIES = libhttp11.la

INCLUDES = \
    -I$(ruby_includes)

CFLAGS = -Wall

http11_so_LDFLAGS = \
	-shared \
    $(ruby_ldflags)

#http11_so_LIBADD = \
#    -lruby18

http11_so_SOURCES = \
    http11.c \
    http11_parser.c \
    tst_cleanup.c \
    tst_delete.c \
    tst_grow_node_free_list.c \
    tst_init.c \
    tst_insert.c \
    tst_search.c

noinst_HEADERS = \
    ext_help.h \
    http11_parser.h \
    tst.h
