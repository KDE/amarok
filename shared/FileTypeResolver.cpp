/****************************************************************************************
 * Copyright (c) 2005 Martin Aumueller <aumueller@reserv.at>                            *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FileTypeResolver.h"

#include <config.h>



#include <QFile>
#include <QFileInfo>
#include <QtDebug>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#ifdef TAGLIB_EXTRAS_FOUND
#include <audiblefile.h>
#include <realmediafile.h>
#endif // TAGLIB_EXTRAS_FOUND
#include <aifffile.h>
#include <asffile.h>
#include <flacfile.h>
#include <mp4file.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#ifdef TAGLIB_OPUS_FOUND
#include <opusfile.h>
#endif // TAGLIB_OPUS_FOUND
#include <oggflacfile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <wavpackfile.h>
#ifdef TAGLIB_MOD_FOUND
#include <modfile.h>
#include <s3mfile.h>
#include <itfile.h>
#include <xmfile.h>
#include <QMimeDatabase>
#include <QMimeType>
#endif // TAGLIB_MOD_FOUND
#pragma GCC diagnostic pop

TagLib::File *Meta::Tag::FileTypeResolver::createFile(TagLib::FileName fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    TagLib::File* result = 0;

    QString fn = QFile::decodeName( fileName );
    QString suffix = QFileInfo( fn ).suffix();
    QMimeType mimetype = db.mimeTypeForFile( fn );

    // -- check by mime type
    if( mimetype.inherits( QLatin1String("audio/mpeg") )
            || mimetype.inherits( QLatin1String("audio/x-mpegurl") )
            || mimetype.inherits( QLatin1String("audio/mpeg") ))
    {
        result = new TagLib::MPEG::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/mp4") )
             || mimetype.inherits( QLatin1String("video/mp4") ) )
    {
        result = new TagLib::MP4::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-ms-wma") )
            || mimetype.inherits( QLatin1String("video/x-ms-asf") )
            || mimetype.inherits( QLatin1String("video/x-msvideo") )
            || mimetype.inherits( QLatin1String("video/x-ms-wmv") ) )
    {
        result = new TagLib::ASF::File(fileName, readProperties, propertiesStyle);
    }
#ifdef TAGLIB_EXTRAS_FOUND
    else if( mimetype.inherits( QLatin1String("audio/vnd.rn-realaudio") )
            || mimetype.inherits( QLatin1String("audio/x-pn-realaudioplugin") )
            || mimetype.inherits( QLatin1String("audio/vnd.rn-realvideo") ) )
    {
        result = new TagLibExtras::RealMedia::File(fileName, readProperties, propertiesStyle);
    }
#endif
#ifdef TAGLIB_OPUS_FOUND
    else if( mimetype.inherits( QLatin1String("audio/opus") )
            || mimetype.inherits( QLatin1String("audio/x-opus+ogg") ) )
    {
        result = new TagLib::Ogg::Opus::File(fileName, readProperties, propertiesStyle);
    }
#endif
    else if( mimetype.inherits( QLatin1String("audio/vorbis") )
            || mimetype.inherits( QLatin1String("audio/x-vorbis+ogg") ) )
    {
        result = new TagLib::Ogg::Vorbis::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-flac+ogg") ) )
    {
        result = new TagLib::Ogg::FLAC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-aiff") ) )
    {
        result = new TagLib::RIFF::AIFF::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-flac") ) )
    {
        result = new TagLib::FLAC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-musepack") ) )
    {
        result = new TagLib::MPC::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-wav") ) )
    {
        result = new TagLib::RIFF::WAV::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-wavpack") ) )
    {
        result = new TagLib::WavPack::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-tta") ) )
    {
        result = new TagLib::TrueAudio::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-speex") )
             || mimetype.inherits( QLatin1String("audio/x-speex+ogg") ) )
    {
        result = new TagLib::Ogg::Speex::File(fileName, readProperties, propertiesStyle);
    }
#ifdef TAGLIB_MOD_FOUND
    else if( mimetype.inherits( QLatin1String("audio/x-mod") ) )
    {
        result = new TagLib::Mod::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-s3m") ) )
    {
        result = new TagLib::S3M::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-it") ) )
    {
        result = new TagLib::IT::File(fileName, readProperties, propertiesStyle);
    }
    else if( mimetype.inherits( QLatin1String("audio/x-xm") ) )
    {
        result = new TagLib::XM::File(fileName, readProperties, propertiesStyle);
    }
#endif

    // -- check by extension
    else if( suffix == QLatin1String("m4a")
        || suffix == QLatin1String("m4b")
        || suffix == QLatin1String("m4p")
        || suffix == QLatin1String("mp4")
        || suffix == QLatin1String("m4v")
        || suffix == QLatin1String("mp4v") )
    {
        result = new TagLib::MP4::File(fileName, readProperties, propertiesStyle);
    }
    else if( suffix == QLatin1String("wav") )
    {
        result = new TagLib::RIFF::WAV::File(fileName, readProperties, propertiesStyle);
    }
    else if( suffix == QLatin1String("wma")
             || suffix == QLatin1String("asf") )
    {
        result = new TagLib::ASF::File(fileName, readProperties, propertiesStyle);
    }
#ifdef TAGLIB_OPUS_FOUND
    // this is currently needed because shared-mime-info database doesn't have opus entry (2013-01)
    else if( suffix == QLatin1String("opus") )
    {
        result = new TagLib::Ogg::Opus::File(fileName, readProperties, propertiesStyle);
    }
#endif

#ifndef Q_WS_WIN
     if( !result )
         qDebug() << QString( "FileTypeResolver: file %1 (mimetype %2) not recognized as "
                "Amarok-compatible" ).arg( fileName, mimetype.name() ).toLocal8Bit().data();
#endif

    if( result && !result->isValid() ) {
        delete result;
        result = 0;
    }

    return result;
}
