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

#include "mimefiletyperesolver.h"

#include <QFile>
#include <QtDebug>
#include <kmimetype.h>

#include <fileref.h>
#include <tfile_helper.h>

#include <flacfile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavpackfile.h>
#include <asffile.h>
#include <realmediafile.h>
#include <audiblefile.h>
#include <wavfile.h>
#include <aifffile.h>
#include <realmediafile.h>
#include <mp4file.h>

TagLib::File *MimeFileTypeResolver::createFile(TagLib::FileName fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    QString fn = QFile::decodeName( fileName );
    KMimeType::Ptr mimetype = KMimeType::findByPath( fn );

    if( mimetype->is( "audio/mpeg" )
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
    else if( mimetype->is( "audio/x-aiff" ) )
    {
        return new TagLib::RIFF::AIFF::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-flac" ) )
    {
        return new TagLib::FLAC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-musepack" ) )
    {
        return new TagLib::MPC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-wav" ) )
    {
        return new TagLib::RIFF::WAV::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-wavpack" ) )
    {
        return new TagLib::WavPack::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-tta" ) )
    {
        return new TagLib::TrueAudio::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype->is( "audio/x-speex" ) || mimetype->is( "audio/x-speex+ogg" ) )
    {
        return new TagLib::TrueAudio::File(fileName, readProperties, propertiesStyle);
    }

    qDebug() << "kmimetype filetype guessing failed for" << fileName;

    return 0;
}
