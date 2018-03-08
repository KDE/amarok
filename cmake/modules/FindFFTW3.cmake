# FindFFTW3
# ---------
#
# Try to locate the FFTW3 library.
# If found, this wil define the following variables:
#
#  FFTW3_FOUND            TRUE if the system has the fftw3 library
#  FFTW3_INCLUDE_DIRS     The fftw3 include directory
#  FFTW3_LIBRARIES        The fftw3 libraries

find_package(PkgConfig)
pkg_check_modules(PC_FFTW3 QUIET fftw3)

find_path(FFTW3_INCLUDE_DIRS
    NAMES fftw3.h
    HINTS ${PC_FFTW3_INCLUDEDIR} ${PC_FFTW3_INCLUDE_DIRS}
)

find_library(FFTW3_LIBRARIES
    NAMES fftw3
    HINTS ${PC_FFTW3_LIBDIR} ${PC_FFTW3_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3 REQUIRED_VARS FFTW3_LIBRARIES FFTW3_INCLUDE_DIRS)

mark_as_advanced(FFTW3_INCLUDE_DIRS FFTW3_LIBRARIES)
set_package_properties(FFTW3 PROPERTIES
    URL "http://www.fftw.org/"
    DESCRIPTION "A C subroutine library for computing the discrete Fourier transform"
)
