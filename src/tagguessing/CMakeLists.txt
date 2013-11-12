## Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
##
## Redistribution and use is allowed according to the terms of the GNU license.
##
## Macros related to external libraries (like LIBCHROMAPRINT_FOUND) should be added in amarok/cmake/modules/
##
## The classes in this directory have already been added to amaroklib
## tagguessing_provider_LIBS is linked to amaroklib in src/CMakeLists.txt
##

## add providers:

#add_subdirectory( musicbrainz )
#set( tagguessing_provider_LIBS tagguessing_musicbrainz )

#if ( LIBCHROMAPRINT_FOUND )
##    add_subdirectory( acustid )
##    set( tagguessing_provider_LIBS tagguessing_acustid )
#endif( LIBCHROMAPRINT_FOUND )
