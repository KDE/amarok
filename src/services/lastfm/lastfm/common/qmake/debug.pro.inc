CONFIG( debug, debug|release ) {
    mac* {
		#speeds up debug builds by only compiling x86
		CONFIG += x86
		CONFIG -= ppc

        !macx-xcode:CONFIG -= app_bundle
	}


    linux* {
        QMAKE_CXXFLAGS_DEBUG = -ggdb
    }

	#if debug is specified first, remove the release stuff! --mxcl
	CONFIG -= release

	VERSION_UPDATE_PATTERN = *.*.*.*
}
else {
    CONFIG += warn_off
	DEFINES += NDEBUG
}
CONFIG( debug, debug|release ) {
    mac* {
		#speeds up debug builds by only compiling x86
		CONFIG += x86
		CONFIG -= ppc

        !macx-xcode:CONFIG -= app_bundle
	}


    linux* {
        QMAKE_CXXFLAGS_DEBUG = -ggdb
    }

	#if debug is specified first, remove the release stuff! --mxcl
	CONFIG -= release

	VERSION_UPDATE_PATTERN = *.*.*.*
}
else {
    CONFIG += warn_off
	DEFINES += NDEBUG
}
