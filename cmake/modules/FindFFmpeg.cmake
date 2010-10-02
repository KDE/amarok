find_path(LIBAVCODEC_INCLUDE_DIR NAMES avcodec.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES libavcodec
)

find_library(LIBAVCODEC_LIBRARY NAMES avcodec
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)

find_path(LIBAVFORMAT_INCLUDE_DIR NAMES avformat.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES libavformat
)

find_library(LIBAVFORMAT_LIBRARY NAMES avformat
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)

if(LIBAVCODEC_INCLUDE_DIR AND LIBAVCODEC_LIBRARY AND
   LIBAVFORMAT_INCLUDE_DIR AND LIBAVFORMAT_LIBRARY)
   set(FFMPEG_FOUND TRUE)
   message(STATUS "Found ffmpeg:")
   message(STATUS "\tlibavcodec: ${LIBAVCODEC_INCLUDE_DIR}, ${LIBAVCODEC_LIBRARY}")
   message(STATUS "\tlibavformat: ${LIBAVFORMAT_INCLUDE_DIR}, ${LIBAVFORMAT_LIBRARY}")
else(LIBAVCODEC_INCLUDE_DIR AND LIBAVCODEC_LIBRARY AND LIBAVFORMAT_INCLUDE_DIR AND LIBAVFORMAT_LIBRARY)
   set(FFMPEG_FOUND FALSE)
   if (FFMPEG_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package ffmpeg")
   endif(FFMPEG_FIND_REQUIRED)
endif(LIBAVCODEC_INCLUDE_DIR AND LIBAVCODEC_LIBRARY AND
      LIBAVFORMAT_INCLUDE_DIR AND LIBAVFORMAT_LIBRARY)

mark_as_advanced(LIBAVCODEC_LIBRARY LIBAVFORMAT_LIBRARY)