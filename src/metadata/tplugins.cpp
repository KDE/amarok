/***************************************************************************
    copyright            : (C) 2005, 2006 by Martin Aumueller
    email                : aumuell@reserv.at
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include <config-amarok.h>
#include <Debug.h>

#include <QFile>

#ifdef KMIMETYPE_RESOLVER
#include <kmimetype.h>
#endif

#include <fileref.h>
#include "tfile_helper.h"

#ifdef HAVE_MP4V2
#include "mp4/taglib_mp4filetyperesolver.h"
#include "mp4/mp4file.h"
#else
#include "m4a/taglib_mp4filetyperesolver.h"
#include "m4a/mp4file.h"
#endif

#include "speex/taglib_speexfiletyperesolver.h"
#include "speex/speexfile.h"
#include "asf/taglib_asffiletyperesolver.h"
#include "asf/asffile.h"
#include "rmff/taglib_realmediafiletyperesolver.h"
#include "rmff/taglib_realmediafile.h"
#include "audible/taglib_audiblefiletyperesolver.h"
#include "audible/taglib_audiblefile.h"
#include "wav/wavfiletyperesolver.h"
#include "wav/wavfile.h"
#include "aac/aacfiletyperesolver.h"

#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <vorbisfile.h>
#include <flacfile.h>
#include <mpcfile.h>
#include <wavpackfile.h>
#include <trueaudiofile.h>


#ifdef KMIMETYPE_RESOLVER
class MimeTypeFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(TagLib::FileName fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

TagLib::File *MimeTypeFileTypeResolver::createFile(TagLib::FileName fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    QString fn = QFile::decodeName( fileName );
    KMimeType::Ptr mimetype = KMimeType::findByPath( fn );

    if( mimetype->is( "audio/aac" )
            || mimetype->is( "audio/mpeg" )
            || mimetype->is( "audio/x-mpegurl" )
            || mimetype->is( "audio/mpeg" ))
    {
        return new TagLib::MPEG::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/mp4" ) || mimetype->is( "video/mp4" ) )
    {
        return new TagLib::MP4::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-ms-wma" )
            || mimetype->is( "video/x-ms-asf" )
            || mimetype->is( "video/x-msvideo" )
            || mimetype->is( "video/x-ms-wmv" ) )
    {
        return new TagLib::ASF::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/vnd.rn-realaudio" )
            || mimetype->is( "audio/x-pn-realaudioplugin" )
            || mimetype->is( "audio/vnd.rn-realvideo" ) )
    {
        return new TagLib::RealMedia::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-vorbis+ogg" ) )
    {
        return new TagLib::Ogg::Vorbis::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-flac+ogg" ) )
    {
        return new TagLib::Ogg::FLAC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-flac" ) )
    {
        return new TagLib::FLAC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-musepack" ) )
    {
        return new TagLib::MPC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-wavpack" ) )
    {
        return new TagLib::WavPack::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-tta" ) )
    {
        return new TagLib::TrueAudio::File(fileName, readProperties, propertiesStyle);
    }

    debug() << "kmimetype filetype guessing failed for" << fileName;

    return 0;
}
#endif

AMAROK_TAGLIB_EXPORT void registerTaglibPlugins()
{
#ifdef KMIMETYPE_RESOLVER
    TagLib::FileRef::addFileTypeResolver(new MimeTypeFileTypeResolver);
#endif
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new ASFFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AACFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new SpeexFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WavFileTypeResolver);
}
