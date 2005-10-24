// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information


#include <taglib/fileref.h>
#include "mp4/taglib_mp4filetyperesolver.h"
#include "wma/taglib_wmafiletyperesolver.h"

void registerTaglibPlugins()
{
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WMAFileTypeResolver);
}
