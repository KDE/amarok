AC_INIT

dnl Fill in your package name and version here
PACKAGE=gst-kio
VERSION=0.1

dnl these AC_DEFINE_UNQUOTED's are necessary for make dist to work
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_DEFINE_UNQUOTED(RELEASE, "$RELEASE")
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(RELEASE)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_PROG_CC
AC_PROG_LIBTOOL

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)

dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

dnl Now we're ready to ask for gstreamer libs and cflags
dnl And we can also ask for the right version of gstreamer
                                                                                
GST_REQUIRED=0.8.0
 
dnl 0.8 required
GST_MAJORMINOR=0.8
PKG_CHECK_MODULES(GST, \
  gstreamer-$GST_MAJORMINOR >= $GST_REQUIRED \
  gstreamer-control-$GST_MAJORMINOR >= $GST_REQUIRED,
  HAVE_GST=yes,HAVE_GST=no)

dnl Give error and exit if we don't have gstreamer
if test "x$HAVE_GST" = "xno"; then
  AC_MSG_ERROR(you need gstreamer development packages installed !)
fi
                                                                                
dnl make GST_CFLAGS and GST_LIBS available
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)
                                                                                
dnl make GST_MAJORMINOR available in Makefile.am
AC_SUBST(GST_MAJORMINOR)

dnl If we need them, we can also use the plugin libraries
PKG_CHECK_MODULES(GST_LIBS, gstreamer-libs-$GST_MAJORMINOR >= $GST_REQUIRED,
                  HAVE_GST_LIBS=yes, HAVE_GST_LIBS=no)
                                                                                
dnl Give a warning if we don't have gstreamer libs
if test "x$HAVE_GST_LIBS" = "xno"; then
  AC_MSG_NOTICE(no GStreamer plugin libs found)
fi
                                                                                
dnl make GST_LIBS_CFLAGS and GST_LIBS_LIBS available
AC_SUBST(GST_LIBS_CFLAGS)
AC_SUBST(GST_LIBS_LIBS)

AC_OUTPUT(Makefile src/Makefile)

