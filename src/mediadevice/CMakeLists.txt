
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

if(IFP_FOUND)
	add_subdirectory(ifp)
endif(IFP_FOUND)

if(NJB_FOUND)
	add_subdirectory(njb)
endif(NJB_FOUND)		

MESSAGE(STATUS "Readd ipod directory")
#add_subdirectory(ipod)


########### install files ###############




#original Makefile.am contents follow:

#if with_libgpod
#    IPOD_SUBDIR = ipod
#endif
#
#if with_ifp
#    IFP_SUBDIR = ifp
#endif
#
#if with_libnjb
#    NJB_SUBDIR = njb
#endif
#
#METASOURCES = AUTO
#
#SUBDIRS = 
#
