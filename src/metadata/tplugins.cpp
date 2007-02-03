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

#include <config.h>
#include <debug.h>

#include <QFile>
#include <kmimetype.h>

#include <fileref.h>
#include <tfile.h>

#ifdef HAVE_MP4V2
#include "mp4/taglib_mp4filetyperesolver.h"
#include "mp4/mp4file.h"
#else
#include "m4a/taglib_mp4filetyperesolver.h"
#include "m4a/mp4file.h"
#endif

#include "trueaudio/taglib_trueaudiofiletyperesolver.h"
#include "trueaudio/ttafile.h"
#include "wavpack/taglib_wavpackfiletyperesolver.h"
#include "wavpack/wvfile.h"
#include "speex/taglib_speexfiletyperesolver.h"
#include "speex/speexfile.h"
#include "wma/taglib_wmafiletyperesolver.h"
#include "wma/wmafile.h"
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


class MimeTypeFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

TagLib::File *MimeTypeFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    QString fn = QFile::decodeName( fileName );
    int accuracy = 0;

    KMimeType::Ptr mimetype = KMimeType::findByFileContent( fn, &accuracy );
    if( accuracy <= 0 )
        mimetype = KMimeType::findByPath( fn );

    if( mimetype->is( "audio/aac" )
            || mimetype->is( "audio/mpeg" )
            || mimetype->is( "audio/mpegurl" )
            || mimetype->is( "audio/x-mpegurl" )
            || mimetype->is( "audio/x-mp3" ))
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
        return new TagLib::WMA::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/vnd.rn-realaudio" )
            || mimetype->is( "audio/x-pn-realaudio" )
            || mimetype->is( "audio/x-pn-realaudioplugin" )
            || mimetype->is( "audio/vnd.rn-realvideo" ) )
    {
        return new TagLib::RealMedia::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/vorbis" ) )
    {
        return new TagLib::Ogg::Vorbis::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-oggflac" ) )
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

    debug() << "kmimetype filetype guessing failed for" << fileName << endl;

    return 0;
}

void registerTaglibPlugins()
{
    //TagLib::FileRef::addFileTypeResolver(new MimeTypeFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WMAFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AACFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WavPackFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new SpeexFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new TTAFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WavFileTypeResolver);
}
