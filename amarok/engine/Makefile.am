INCLUDES = $(all_includes) -I$(kde_includes)/arts $(taglib_includes) $(CFLAGS_GST)
METASOURCES = AUTO
noinst_LIBRARIES =  libengine.a
libengine_a_SOURCES = enginebase.cpp artsengine.cpp gstengine.cpp
libengine_a_LIBADD = $(LIB_GST)
noinst_HEADERS = enginebase.h artsengine.h gstengine.h
