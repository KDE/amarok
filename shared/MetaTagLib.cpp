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
#include "TagsFromFileNameGuesser.h"
#include <config.h>

#include <KEncodingProber>

#include <QImage>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMutex>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QString>
#include <QTime>

#include "FileTypeResolver.h"
#include "MetaReplayGain.h"
#include "tag_helpers/TagHelper.h"
#include "tag_helpers/StringHelper.h"

//Taglib:
#include <audioproperties.h>

#ifdef TAGLIB_EXTRAS_FOUND
#include <audiblefiletyperesolver.h>
#include <realmediafiletyperesolver.h>
#endif // TAGLIB_EXTRAS_FOUND


namespace Meta
{
    namespace Tag
    {
        QMutex s_mutex;

        static void addRandomness( QCryptographicHash *md5 );

        /** Get a taglib fileref for a path */
        static TagLib::FileRef getFileRef( const QString &path );

        /** Returns a byte vector that can be used to generate the unique id based on the tags. */
        static TagLib::ByteVector generatedUniqueIdHelper( const TagLib::FileRef &fileref );

        static QString generateUniqueId( const QString &path );

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
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
    md5->addData( QString::number( QRandomGenerator::global()->generate() ).toLatin1() );
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
Meta::Tag::generateUniqueId( const QString &path )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QFile qfile( path );
    QByteArray size;
    md5.addData( size.setNum( qfile.size() ) );

    TagLib::FileRef fileref = getFileRef( path );
    TagLib::ByteVector bv = generatedUniqueIdHelper( fileref );
    md5.addData( bv.data(), bv.size() );

    if( qfile.open( QIODevice::ReadOnly ) )
    {
        char databuf[16384];
        int readlen;
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

    return QString::fromLatin1( md5.result().toHex() );
}


// --------- file type resolver ----------

/** Will ensure that we have our file type resolvers added */
static void ensureFileTypeResolvers()
{
    static bool alreadyAdded = false;
    if( !alreadyAdded ) {
        alreadyAdded = true;

#ifdef TAGLIB_EXTRAS_FOUND
        TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
        TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
#endif
        TagLib::FileRef::addFileTypeResolver(new Meta::Tag::FileTypeResolver());
    }
}

// ----------------------- reading ------------------------

Meta::FieldHash
Meta::Tag::readTags( const QString &path, bool /*useCharsetDetector*/ )
{
    Meta::FieldHash result;

    // we do not rely on taglib being thread safe especially when writing the same file from different threads.
    QMutexLocker locker( &s_mutex );
    ensureFileTypeResolvers();

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

        result.insert( Meta::valFormat, tagHelper->fileType() );
        result.insert( tagHelper->tags() );
        delete tagHelper;
    }

    TagLib::AudioProperties *properties = fileref.audioProperties();
    if( properties )
    {
        if( !result.contains( Meta::valBitrate ) && properties->bitrate() )
            result.insert( Meta::valBitrate, properties->bitrate() );
        if( !result.contains( Meta::valLength ) && properties->lengthInMilliseconds() )
            result.insert( Meta::valLength, properties->lengthInMilliseconds() );
        if( !result.contains( Meta::valSamplerate ) && properties->sampleRate() )
            result.insert( Meta::valSamplerate, properties->sampleRate() );
    }

    //If tags doesn't contains title and artist, try to guess It from file name
    if( !result.contains( Meta::valTitle ) ||
        result.value( Meta::valTitle ).toString().isEmpty() )
    {
        Meta::FieldHash secondResult = TagGuesser::guessTags( path );
        // We got here because title is empty. But what if other fields are OK? They should
        // probably be preferred over guessed values - unless, if they are empty. Existing fields
        // are overwritten with Hash::insert, so first remove empty fields from result, then
        // swap read tags and guessed fields, and then overwrite those guessed fields that have
        // a perhaps ok value in the read tags.
        for( auto t : { Meta::valTitle, Meta::valAlbum, Meta::valArtist, Meta::valTrackNr } )
        {
            if( result.value( t ).toString().isEmpty() )
                result.remove( t );
        }
        result.swap( secondResult );
        result.insert( secondResult );
    }

    //we didn't set a FileType till now, let's look it up via FileExtension
    if( !result.contains( Meta::valFormat ) )
    {
        QString ext = path.mid( path.lastIndexOf( QLatin1Char('.') ) + 1 );
        result.insert( Meta::valFormat, Amarok::FileTypeSupport::fileType( ext ) );
    }

    QFileInfo fileInfo( path );
    result.insert( Meta::valFilesize, fileInfo.size() );
    result.insert( Meta::valModified, fileInfo.lastModified() );

    if( !result.contains( Meta::valUniqueId ) )
        result.insert( Meta::valUniqueId, generateUniqueId( path ) );

    // compute bitrate if it is not already set and we know length
    if( !result.contains( Meta::valBitrate ) && result.contains( Meta::valLength ) )
        result.insert( Meta::valBitrate, ( fileInfo.size() * 8 * 1000 ) /
                       ( result.value( Meta::valLength ).toInt() * 1024 ) );

    return result;
}

QImage
Meta::Tag::embeddedCover( const QString &path )
{
    // we do not rely on taglib being thread safe especially when writing the same file from different threads.
    QMutexLocker locker( &s_mutex );

    ensureFileTypeResolvers();
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

void
Meta::Tag::writeTags( const QString &path, const FieldHash &changes, bool writeStatistics )
{
    FieldHash data = changes;

    if( !writeStatistics )
    {
        data.remove( Meta::valFirstPlayed );
        data.remove( Meta::valLastPlayed );
        data.remove( Meta::valPlaycount );
        data.remove( Meta::valScore );
        data.remove( Meta::valRating );
    }

    // we do not rely on taglib being thread safe especially when writing the same file from different threads.
    QMutexLocker locker( &s_mutex );

    ensureFileTypeResolvers();
    TagLib::FileRef fileref = getFileRef( path );
    if( fileref.isNull() || data.isEmpty() )
        return;

    QScopedPointer<TagHelper> tagHelper( selectHelper( fileref, true ) );
    if( !tagHelper )
        return;

    if( tagHelper->setTags( data ) )
        fileref.save();
}

void
Meta::Tag::setEmbeddedCover( const QString &path, const QImage &cover )
{
    // we do not rely on taglib being thread safe especially when writing the same file from different threads.
    QMutexLocker locker( &s_mutex );
    ensureFileTypeResolvers();

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

#undef Qt4QStringToTString
