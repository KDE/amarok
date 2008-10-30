CONFIG( breakpad ) {
    linux* {
		QMAKE_CXXFLAGS_RELEASE = -Os -freorder-blocks -fno-reorder-functions -fomit-frame-pointer -gstabs+
	}
    mac* {
		QMAKE_CXXFLAGS_RELEASE = -Os -g -fomit-frame-pointer
	}
    win32 {
        QMAKE_CXXFLAGS_RELEASE = -Zi -Oy -Ox -GL -MD -DNDEBUG
        QMAKE_LFLAGS_RELEASE = /ltcg
    }

	QMAKE_CFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE

	unix {
		# optimise mpg decoding
		QMAKE_CFLAGS_RELEASE += -O3
		QMAKE_CFLAGS_RELEASE -= -Os
	}
	
	INCLUDEPATH += $$COMMON_LIBS_DIR/breakpad/src
}
CONFIG( breakpad ) {
    linux* {
		QMAKE_CXXFLAGS_RELEASE = -Os -freorder-blocks -fno-reorder-functions -fomit-frame-pointer -gstabs+
	}
    mac* {
		QMAKE_CXXFLAGS_RELEASE = -Os -g -fomit-frame-pointer
	}
    win32 {
        QMAKE_CXXFLAGS_RELEASE = -Zi -Oy -Ox -GL -MD -DNDEBUG
        QMAKE_LFLAGS_RELEASE = /ltcg
    }

	QMAKE_CFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE

	unix {
		# optimise mpg decoding
		QMAKE_CFLAGS_RELEASE += -O3
		QMAKE_CFLAGS_RELEASE -= -Os
	}
	
	INCLUDEPATH += $$COMMON_LIBS_DIR/breakpad/src
}
