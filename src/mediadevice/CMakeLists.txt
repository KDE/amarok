
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

if(IFP_FOUND)
	add_subdirectory(ifp)
endif(IFP_FOUND)

if(NJB_FOUND)
	add_subdirectory(njb)
endif(NJB_FOUND)		

MESSAGE(STATUS "Read ipod directory")
#add_subdirectory(ipod)


