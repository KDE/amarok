#ifndef TFILE_HELPER_H
#define TFILE_HELPER_H

#include "config-amarok.h"
#include <tfile.h>

// need to make everything compile against old versions of taglib
// where TagLib::FileName wasn't typedef'd to const char *
#ifdef HAVE_TAGLIB_FILENAME
#define TAGLIB_FILENAME TagLib::FileName
#else
#define TAGLIB_FILENAME const char *
#endif

#endif // TFILE_HELPER_H
