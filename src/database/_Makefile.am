noinst_LTLIBRARIES = libdbengine.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src/plugin \
    -I$(top_srcdir)/amarok/src \
    $(all_includes)

if with_sqlite
    SQLITE_DBENGINE_SUBDIR = sqlite
endif

if with_mysql
    MYSQL_DBENGINE_SUBDIR = mysql
endif

if with_postgresql
    POSTGRESQL_DBENGINE_SUBDIR = postgresql
endif

libengine_la_SOURCES = \
    dbenginebase.cpp

noinst_HEADERS = dbenginebase.h

METASOURCES = \
    AUTO

SUBDIRS = . \
    void \
    $(SQLITE_DBENGINE_SUBDIR) \
    $(MYSQL_DBENGINE_SUBDIR) \
    $(POSTGRESQL_DBENGINE_SUBDIR)

