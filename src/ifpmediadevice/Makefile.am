INCLUDES           = $(all_includes) $(IFP_CFLAGS) -I.. -I../engine
METASOURCES        = AUTO

libifpmediadevice_la_LIBADD = -lusb -lifp

libifpmediadevice_la_LDFLAGS  = $(all_libraries)

noinst_LTLIBRARIES = libifpmediadevice.la

libifpmediadevice_la_SOURCES =  \
                     ifpmediadevice.cpp

noinst_HEADERS = \
                     ifpmediadevice.h
