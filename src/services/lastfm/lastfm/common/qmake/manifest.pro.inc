
contains( TEMPLATE, vclib ):tinkywinky = no
contains( TEMPLATE, vcapp ):tinkywinky = no

!contains( tinkywinky, no ) {
    # breaks visual studio build system
    # NOTE always build release builds with nmake template!
win32 {

	# Build pdb and map files also for release builds
    QMAKE_LFLAGS_RELEASE += /DEBUG /MAP
    
    # Make the linker tell us what it's doing
    QMAKE_LFLAGS_DEBUG += /VERBOSE:LIB
    
    # Removes warnings about standard library usage
    DEFINES += _CRT_SECURE_NO_DEPRECATE _SCL_SECURE_NO_DEPRECATE
    
    # Prevents windows.h from including winsock.h as it will clash with
    # Winsock2.h which is included by the player listener.
    DEFINES += _WINSOCKAPI_

    # Here's the result of a goddam week spent debugging a heap corruption in msvcp80.dll.
    # We can't let VS embed its default manifest into our binaries as it will then load
    # up whatever matching CRT it finds in Windows/WinSxS. Since version 1433 however,
    # the system CRTs got fucked up by MS so we need to force our binaries to only use the
    # CRT files we ship. By adding the handcoded crtdep.manifest which has the publicKeyToken
    # attribute taken out and ship a CRT assembly whose manifest file also has this taken out,
    # we can accompish this.
    CONFIG -= embed_manifest_exe
    CONFIG -= embed_manifest_dll
    QMAKE_LFLAGS += /MANIFEST:NO

    # Unfortunately qmake doesn't give us the final target name in full
    # so we need to piece it together
    !staticlib {
        contains( TEMPLATE, app ) {
            FILE_EXT = exe
        } else {
            FILE_EXT = dll
        }

        isEmpty( VERSION ) {
            OUTPUT_RESOURCE = "$${DESTDIR}/$${TARGET}.$${FILE_EXT};$${LITERAL_HASH}1"
        } else {
            OUTPUT_RESOURCE = "$${DESTDIR}/$${TARGET}1.$${FILE_EXT};$${LITERAL_HASH}1"
        }

        CONFIG(release, release|debug) {
            CRTDEP_FILE = $${UNICORNPATH}/crtdep.manifest
        } else {
            CRTDEP_FILE = $${UNICORNPATH}/crtdepd.manifest
        }

        QMAKE_POST_LINK = mt.exe -manifest $${CRTDEP_FILE} \
            -outputresource:$${OUTPUT_RESOURCE}

        # For Vista UAC to work correctly, we also need to merge in a trustInfo section
        # in our exes. This is to specify to the OS to run as invoker and not elevated.
        contains( TEMPLATE, app ) {
            #CONFIG(release, release|debug) {
                QMAKE_POST_LINK += && mt.exe -manifest trustInfo.manifest \
                    -updateresource:$${DESTDIR}/$${TARGET}.exe;$${LITERAL_HASH}1							 
            #} else {
            #    QMAKE_POST_LINK += mt.exe -manifest trustInfo.manifest \
            #        -outputresource:"$${DESTDIR}/$${TARGET}d.exe;$${LITERAL_HASH}1"
            #}
        }
    }
}
}

contains( TEMPLATE, vclib ):tinkywinky = no
contains( TEMPLATE, vcapp ):tinkywinky = no

!contains( tinkywinky, no ) {
    # breaks visual studio build system
    # NOTE always build release builds with nmake template!
win32 {

	# Build pdb and map files also for release builds
    QMAKE_LFLAGS_RELEASE += /DEBUG /MAP
    
    # Make the linker tell us what it's doing
    QMAKE_LFLAGS_DEBUG += /VERBOSE:LIB
    
    # Removes warnings about standard library usage
    DEFINES += _CRT_SECURE_NO_DEPRECATE _SCL_SECURE_NO_DEPRECATE
    
    # Prevents windows.h from including winsock.h as it will clash with
    # Winsock2.h which is included by the player listener.
    DEFINES += _WINSOCKAPI_

    # Here's the result of a goddam week spent debugging a heap corruption in msvcp80.dll.
    # We can't let VS embed its default manifest into our binaries as it will then load
    # up whatever matching CRT it finds in Windows/WinSxS. Since version 1433 however,
    # the system CRTs got fucked up by MS so we need to force our binaries to only use the
    # CRT files we ship. By adding the handcoded crtdep.manifest which has the publicKeyToken
    # attribute taken out and ship a CRT assembly whose manifest file also has this taken out,
    # we can accompish this.
    CONFIG -= embed_manifest_exe
    CONFIG -= embed_manifest_dll
    QMAKE_LFLAGS += /MANIFEST:NO

    # Unfortunately qmake doesn't give us the final target name in full
    # so we need to piece it together
    !staticlib {
        contains( TEMPLATE, app ) {
            FILE_EXT = exe
        } else {
            FILE_EXT = dll
        }

        isEmpty( VERSION ) {
            OUTPUT_RESOURCE = "$${DESTDIR}/$${TARGET}.$${FILE_EXT};$${LITERAL_HASH}1"
        } else {
            OUTPUT_RESOURCE = "$${DESTDIR}/$${TARGET}1.$${FILE_EXT};$${LITERAL_HASH}1"
        }

        CONFIG(release, release|debug) {
            CRTDEP_FILE = $${UNICORNPATH}/crtdep.manifest
        } else {
            CRTDEP_FILE = $${UNICORNPATH}/crtdepd.manifest
        }

        QMAKE_POST_LINK = mt.exe -manifest $${CRTDEP_FILE} \
            -outputresource:$${OUTPUT_RESOURCE}

        # For Vista UAC to work correctly, we also need to merge in a trustInfo section
        # in our exes. This is to specify to the OS to run as invoker and not elevated.
        contains( TEMPLATE, app ) {
            #CONFIG(release, release|debug) {
                QMAKE_POST_LINK += && mt.exe -manifest trustInfo.manifest \
                    -updateresource:$${DESTDIR}/$${TARGET}.exe;$${LITERAL_HASH}1							 
            #} else {
            #    QMAKE_POST_LINK += mt.exe -manifest trustInfo.manifest \
            #        -outputresource:"$${DESTDIR}/$${TARGET}d.exe;$${LITERAL_HASH}1"
            #}
        }
    }
}
}