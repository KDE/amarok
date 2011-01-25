/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
 *             (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
 *             (C) 2010 Sergey Ivanov <123kash@gmail.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "MetaTagLib.h"
#include "FileType.h"
#include "MetaReplayGain.h"
#include "TagsFromFileNameGuesser.h"

#ifndef UTILITIES_BUILD
#include "amarokconfig.h"
#endif

#include <KEncodingProber>
#include <QImage>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>
#include <QDebug>

//Taglib:
#include <audioproperties.h>
#include <fileref.h>

// TagHelpers
#include "tag_helpers/TagHelper.h"
#include "tag_helpers/StringHelper.h"

namespace Meta
{
    namespace Tag
    {
        QMutex s_mutex;

        static TagLib::FileRef getFileRef( const QString &path );

        static void addRandomness( QCryptographicHash *md5 );

        /** Returns a byte vector that can be used to generate the unique id based on the tags. */
        static TagLib::ByteVector generatedUniqueIdHelper( const TagLib::FileRef &fileref );
        static QString generateUniqueId( const QString &path, const TagLib::FileRef &fileref );

    }
}

TagLib::FileRef
Meta::Tag::getFileRef( const QString &path )
{
#ifdef Q_OS_WIN32
    const wchar_t *encodedName = reinterpret_cast< const wchar_t * >( path.utf16() );
#else
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t *encodedName = reinterpret_cast< const wchar_t * >( path.utf16() );
#else
    QByteArray fileName = QFile::encodeName( path );
    const char *encodedName = fileName.constData(); // valid as long as fileName exists
#endif
#endif

    // Tests reveal the following:
    //
    // TagLib::AudioProperties   Relative Time Taken
    //
    //  No AudioProp Reading        1
    //  Fast                        1.18
    //  Average                     Untested
    //  Accurate                    Untested

    return TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );
}


// ----------------------- unique id ------------------------


void
Meta::Tag::addRandomness( QCryptographicHash *md5 )
{
    //md5 has size of file already added for some little extra randomness for the hash
    qsrand( QTime::currentTime().msec() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
}

TagLib::ByteVector
Meta::Tag::generatedUniqueIdHelper( const TagLib::FileRef &fileref )
{
    TagLib::ByteVector bv;

    TagHelper *tagHelper = selectHelper( fileref );

    if( tagHelper )
    {
        bv = tagHelper->render();
        delete tagHelper;
    }

    return bv;
}

QString
Meta::Tag::generateUniqueId( const QString &path, const TagLib::FileRef &fileref )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QFile qfile( path );
    QByteArray size;
    md5.addData( size.setNum( qfile.size() ) );

    TagLib::ByteVector bv = generatedUniqueIdHelper( fileref );
    md5.addData( bv.data(), bv.size() );

    char databuf[16384];
    int readlen = 0;
    QString returnval;

    if( qfile.open( QIODevice::ReadOnly ) )
    {
        if( ( readlen = qfile.read( databuf, 16384 ) ) > 0 )
        {
            md5.addData( databuf, readlen );
            qfile.close();
        }
        else
        {
            qfile.close();
            addRandomness( &md5 );
        }
    }
    else
        addRandomness( &md5 );

    return QString( md5.result().toHex() );
}

// ----------------------- reading ------------------------

Meta::FieldHash
Meta::Tag::readTags( const QString &path, bool /*useCharsetDetector*/ )
{
    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    Meta::FieldHash result;

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return result;

    Meta::ReplayGainTagMap replayGainTags = Meta::readReplayGainTags( fileref );
    if( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        result.insert( Meta::valTrackGain, replayGainTags[Meta::ReplayGain_Track_Gain] );
    if( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
        result.insert( Meta::valTrackGainPeak, replayGainTags[Meta::ReplayGain_Track_Peak] );

    // strangely: the album gain defaults to the track gain
    if( replayGainTags.contains( Meta::ReplayGain_Album_Gain ) )
        result.insert( Meta::valAlbumGain, replayGainTags[Meta::ReplayGain_Album_Gain] );
    else if( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        result.insert( Meta::valAlbumGain, replayGainTags[Meta::ReplayGain_Track_Gain] );
    if( replayGainTags.contains( Meta::ReplayGain_Album_Peak ) )
        result.insert( Meta::valAlbumGainPeak, replayGainTags[Meta::ReplayGain_Album_Peak] );
    else if( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
        result.insert( Meta::valAlbumGainPeak, replayGainTags[Meta::ReplayGain_Track_Peak] );

    TagHelper *tagHelper = selectHelper( fileref );
    if( tagHelper )
    {
        if( 0/* useCharsetDetector */ )
        {
            KEncodingProber prober;
            if( prober.feed( tagHelper->testString() ) != KEncodingProber::NotMe )
                Meta::Tag::setCodecByName( prober.encoding() );
        }

        result.insert( Meta::valFiletype, tagHelper->fileType() );
        result.unite( tagHelper->tags() );
        delete tagHelper;
    }

    //If tags doesn't contains title and artist, try to guess It from file name
    if( !result.contains( Meta::valTitle ) ||
        result.value( Meta::valTitle ).toString().isEmpty() )
        result.unite( TagGuesser::guessTags( path ) );

    //we didn't set a FileType till now, let's look it up via FileExtension
    if( !result.contains( Meta::valFiletype ) )
    {
        QString ext = path.mid( path.lastIndexOf( '.' ) + 1 );
        result.insert( Meta::valFiletype, Amarok::FileTypeSupport::fileType( ext ) );
    }

    if( fileref.audioProperties() )
    {
        result.insert( Meta::valBitrate, fileref.audioProperties()->bitrate() );
        result.insert( Meta::valLength, fileref.audioProperties()->length() * 1000 );
        result.insert( Meta::valSamplerate, fileref.audioProperties()->sampleRate() );
    }

    QFileInfo fileInfo( path );
    result.insert( Meta::valFilesize, fileInfo.size() );
    result.insert( Meta::valModified, fileInfo.lastModified() );

    if( !result.contains( Meta::valUniqueId ) )
        result.insert( Meta::valUniqueId, generateUniqueId( path, fileref ) );

    return result;
}

#ifndef UTILITIES_BUILD
// the utilities don't need to handle images

QImage
Meta::Tag::embeddedCover( const QString &path )
{
    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return QImage();

    QImage img;

    TagHelper *tagHelper = selectHelper( fileref );
    if( tagHelper )
    {
        img = tagHelper->embeddedCover();
        delete tagHelper;
    }

    return img;
}

#endif

void
Meta::Tag::writeTags( const QString &path, const FieldHash &changes )
{
    FieldHash data = changes;
#ifndef UTILITIES_BUILD
    // depending on the configuration we might not want to write back anything
    if( !AmarokConfig::writeBack() )
        return;

    // depending on the configuration we might not want to write back statistics
    if( !AmarokConfig::writeBackStatistics() )
    {
        data.remove( Meta::valFirstPlayed );
        data.remove( Meta::valLastPlayed );
        data.remove( Meta::valPlaycount );
        data.remove( Meta::valScore );
    }
#endif

    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() || data.isEmpty() )
        return;

    TagHelper *tagHelper = selectHelper( fileref, true );
    if( !tagHelper )
        return;

    if( tagHelper->setTags( data ) )
        fileref.save();

    delete tagHelper;
}

#ifndef UTILITIES_BUILD

void
Meta::Tag::setEmbeddedCover( const QString &path, const QImage &cover )
{
    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return;

    TagHelper *tagHelper = selectHelper( fileref, true );
    if( !tagHelper )
        return;

    if( tagHelper->setEmbeddedCover( cover ) )
        fileref.save();

    delete tagHelper;
}

#endif

#undef Qt4QStringToTString

