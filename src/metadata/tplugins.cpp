// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information


#include <taglib/fileref.h>

#include <config.h>

#ifdef HAVE_MP4V2
#include "mp4/taglib_mp4filetyperesolver.h"
#else
#include "m4a/taglib_mp4filetyperesolver.h"
#endif

#include "wma/taglib_wmafiletyperesolver.h"
#include "rmff/taglib_realmediafiletyperesolver.h"
#include "audible/taglib_audiblefiletyperesolver.h"
#include "aac/aacfiletyperesolver.h"

void registerTaglibPlugins()
{
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WMAFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AACFileTypeResolver);
}
