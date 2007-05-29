noinst_LTLIBRARIES = \
    libsqlite.la

KDE_CFLAGS = \
    -w

libsqlite_la_CFLAGS = \
    $(all_includes) \
    -DTHREADSAFE=1 \
    -DHAVE_USLEEP=1

libsqlite_la_LDFLAGS = \
    $(LIBPTHREAD)

libsqlite_la_SOURCES = \
    sqlite3.c

