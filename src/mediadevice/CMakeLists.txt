
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

#if(IFP_FOUND)
#	add_subdirectory(ifp)
#endif(IFP_FOUND)

#if(NJB_FOUND)
#	add_subdirectory(njb)
#endif(NJB_FOUND)

#if(MTP_FOUND)
#    MESSAGE(STATUS "libmtp found, including MTP device plugin")
#    add_subdirectory(mtp)
#endif(MTP_FOUND)

MESSAGE(STATUS "Including generic device plugin")
add_subdirectory(generic)

#MESSAGE(STATUS "Read ipod directory")
#add_subdirectory(ipod)


