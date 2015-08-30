include_directories(
                    
                    
                    ${Amarok_SOURCE_DIR}/src
                   )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

#add_subdirectory( daap )           #FIXME: disabled temporarily for KF5 porting: Port this to KF5 after the rest of the code has been ported to the other KF5 components.

if( NOT WIN32)
#add_subdirectory( audiocd )        #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
endif()

#add_subdirectory( ipodcollection ) #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
#add_subdirectory( mtpcollection )  #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
#add_subdirectory( umscollection )  #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
add_subdirectory( db )

# only build playdarcollection if QJson was found
if( QJSON_FOUND )
#   add_subdirectory( playdarcollection )   #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
endif()
#add_subdirectory( upnpcollection )         #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
#add_subdirectory( nepomukcollection )      #FIXME: disabled temporarily for KF5 porting: Port this to KF5::Plasma after the rest of the code has been ported to the other KF5 components.
