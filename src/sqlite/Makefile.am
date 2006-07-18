noinst_LTLIBRARIES = \
    libsqlite.la

KDE_CFLAGS = \
    -w

libsqlite_la_CFLAGS = \
    $(all_includes) \
    -DTHREADSAFE=1 \
    -DHAVE_USLEEP=1 \
    -fno-strict-aliasing

libsqlite_la_LDFLAGS = \
    $(LIBPTHREAD)

libsqlite_la_SOURCES = \
    alter.c \
    analyze.c \
    attach.c \
    auth.c \
    btree.c \
    build.c \
    callback.c \
    complete.c \
    date.c \
    delete.c \
    expr.c \
    func.c \
    hash.c \
    insert.c \
    legacy.c \
    main.c \
    opcodes.c \
    os.c \
    os_os2.c \
    os_unix.c \
    os_win.c \
    pager.c \
    parse.c \
    pragma.c \
    prepare.c \
    printf.c \
    random.c \
    select.c \
    table.c \
    tokenize.c \
    trigger.c \
    update.c \
    utf.c \
    util.c \
    vacuum.c \
    vdbe.c \
    vdbeapi.c \
    vdbeaux.c \
    vdbefifo.c \
    vdbemem.c \
    where.c

