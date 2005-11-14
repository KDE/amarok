// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information


#include <taglib/fileref.h>

#include <config.h>

#ifdef HAVE_MP4V2
#include "mp4/taglib_mp4filetyperesolver.h"
#endif

#include "wma/taglib_wmafiletyperesolver.h"
#include "audible/taglib_audiblefiletyperesolver.h"

void registerTaglibPlugins()
{
#ifdef HAVE_MP4V2
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
#endif
    TagLib::FileRef::addFileTypeResolver(new WMAFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
}
