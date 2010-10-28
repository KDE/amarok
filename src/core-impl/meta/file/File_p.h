/****************************************************************************************
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_META_FILE_P_H
#define AMAROK_META_FILE_P_H

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core-impl/meta/file/TagLibUtils.h"
#include "shared/MetaReplayGain.h"
#include "core/statistics/StatisticsProvider.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QWeakPointer>
#include <QSet>
#include <QString>
#include <QTextCodec>

#include <KEncodingProber>
#include <KLocale>

// Taglib Includes
#include <fileref.h>
#include <tag.h>
#include <flacfile.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <mpcfile.h>
#include <apetag.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <tlist.h>
#include <tstring.h>
#include <vorbisfile.h>
#include <mp4file.h>
#include <attachedpictureframe.h>

namespace Capabilities
{
    class LastfmReadLabelCapability;
}

namespace MetaFile
{

//d-pointer implementation

struct MetaData
{
    MetaData()
        : discNumber( 0 )
        , trackNumber( 0 )
        , length( 0 )
        , fileSize( 0 )
        , sampleRate( 0 )
        , bitRate( 0 )
        , year( 0 )
        , bpm( 0.0 )
        , trackGain( 0.0 )
        , trackPeak( 0.0 )
        , albumGain( 0.0 )
        , albumPeak( 0.0 )
        , embeddedImage( false )
    { }
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString comment;
    QString composer;
    QString genre;
    QDateTime created;
    int discNumber;
    int trackNumber;
    qint64 length;
    int fileSize;
    int sampleRate;
    int bitRate;
    int year;
    qreal bpm;
    qreal trackGain;
    qreal trackPeak;
    qreal albumGain;
    qreal albumPeak;
    bool embeddedImage;
};

class Track::Private : public QObject
{
public:
    Private( Track *t )
        : QObject()
        , url()
        , batchUpdate( false )
        , album()
        , artist()
        , provider( 0 )
        , track( t )
    {}

public:
    KUrl url;
    bool batchUpdate;
    Meta::AlbumPtr album;
    Meta::ArtistPtr artist;
    Meta::GenrePtr genre;
    Meta::ComposerPtr composer;
    Meta::YearPtr year;
    Statistics::StatisticsProvider *provider;
    QWeakPointer<Capabilities::LastfmReadLabelCapability> readLabelCapability;

    void readMetaData();
    QVariantMap changes;

    void writeMetaData() { DEBUG_BLOCK Meta::Field::writeFields( track->getFileRef( url ), changes ); changes.clear(); readMetaData(); }
    MetaData m_data;

private:
    TagLib::FileRef getFileRef();
    Track *track;
};

void Track::Private::readMetaData()
{
    QFileInfo fi( url.isLocalFile() ? url.toLocalFile() : url.path() );
    m_data.created = fi.created();

#define strip( x ) TStringToQString( x ).trimmed()
    TagLib::FileRef fileRef = track->getFileRef( url );

    TagLib::Tag *tag = 0;
    if( !fileRef.isNull() )
        tag = fileRef.tag();

    if( tag )
    {
        m_data.title = strip( tag->title() );
        m_data.artist = strip( tag->artist() );
        m_data.album = strip( tag->album() );
        m_data.comment = strip( tag->comment() );
        m_data.genre = strip( tag->genre() );
        m_data.trackNumber = tag->track();
        m_data.year = tag->year();
    }
    if( !fileRef.isNull() )
    {
        if( fileRef.audioProperties() )
        {
            m_data.bitRate = fileRef.audioProperties()->bitrate();
            m_data.sampleRate = fileRef.audioProperties()->sampleRate();
            m_data.length = fileRef.audioProperties()->length() * 1000;
        }

        Meta::ReplayGainTagMap map = Meta::readReplayGainTags( fileRef );
        if ( map.contains( Meta::ReplayGain_Track_Gain ) )
            m_data.trackGain = map[Meta::ReplayGain_Track_Gain];
        if ( map.contains( Meta::ReplayGain_Track_Peak ) )
            m_data.trackPeak = map[Meta::ReplayGain_Track_Peak];
        if ( map.contains( Meta::ReplayGain_Album_Gain ) )
            m_data.albumGain = map[Meta::ReplayGain_Album_Gain];
        else
            m_data.albumGain = m_data.trackGain;
        if ( map.contains( Meta::ReplayGain_Album_Peak ) )
            m_data.albumPeak = map[Meta::ReplayGain_Album_Peak];
        else
            m_data.albumPeak = m_data.trackPeak;
    }
    //This is pretty messy...
    QString disc;
    m_data.bpm = -1.0;
    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileRef.file() ) )
    {
        if( file->ID3v2Tag() )
        {
            const TagLib::ID3v2::FrameListMap flm = file->ID3v2Tag()->frameListMap();
            if( !flm[ "TPOS" ].isEmpty() )
                disc = strip( flm[ "TPOS" ].front()->toString() );

            if( !flm[ "TCOM" ].isEmpty() )
                m_data.composer = strip( flm[ "TCOM" ].front()->toString() );

            if( !flm[ "TPE2" ].isEmpty() )
                m_data.albumArtist = strip( flm[ "TPE2" ].front()->toString() );

            if( !flm[ "TBPM" ].isEmpty() )
                m_data.bpm = TStringToQString( flm[ "TBPM" ].front()->toString() ).toFloat();

            if ( !file->ID3v2Tag()->frameListMap()["APIC"].isEmpty() )
            {
                TagLib::ID3v2::FrameList apicList = file->ID3v2Tag()->frameListMap()["APIC"];
                TagLib::ID3v2::FrameList::ConstIterator iter;
                for( iter = apicList.begin(); iter != apicList.end(); ++iter )
                {
                    TagLib::ID3v2::AttachedPictureFrame* currFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*iter);
                    if( currFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover ||
                        currFrame->type() == TagLib::ID3v2::AttachedPictureFrame::Other )
                    {
                        m_data.embeddedImage = true;
                    }
                }
            }

        }
        if( AmarokConfig::useCharsetDetector() && tag )
        {
            TagLib::String metaData = tag->title() + tag->artist() + tag->album() + tag->comment();
            const char* buf = metaData.toCString();
            size_t len = strlen( buf );
            KEncodingProber prober;
            KEncodingProber::ProberState result = prober.feed( buf, len );
            QString track_encoding( prober.encoding() );
            if ( result != KEncodingProber::NotMe )
            {
                /*  for further information please refer to:
                    http://doc.trolltech.com/4.4/qtextcodec.html
                    http://www.mozilla.org/projects/intl/chardet.html
                */
                if ( ( track_encoding.toUtf8() == "gb18030" ) || ( track_encoding.toUtf8() == "big5" )
                    || ( track_encoding.toUtf8() == "euc-kr" ) || ( track_encoding.toUtf8() == "euc-jp" )
                    || ( track_encoding.toUtf8() == "koi8-r" ) )
                {
                    debug () << "Final Codec Name:" << track_encoding.toUtf8();
                    QTextCodec *codec = QTextCodec::codecForName( track_encoding.toUtf8() );
                    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
                    QTextCodec::setCodecForCStrings( utf8codec );
                    if ( codec != 0 )
                    {
                        m_data.title = codec->toUnicode( m_data.title.toLatin1() );
                        m_data.artist = codec->toUnicode( m_data.artist.toLatin1() );
                        m_data.album = codec->toUnicode( m_data.album.toLatin1() );
                        m_data.comment = codec->toUnicode( m_data.comment.toLatin1() );
                        debug() << "track Info Decoded!";
                    }
                }
                else
                {
                    debug() << "possible encoding: " << track_encoding.toUtf8();
                    debug() << "encoding decoded as UTF-8";
                }
            }
        }
    }

    else if( TagLib::Ogg::Vorbis::File *file = dynamic_cast< TagLib::Ogg::Vorbis::File *>( fileRef.file() ) )
    {
        if( file->tag() )
        {
            const TagLib::Ogg::FieldListMap flm = file->tag()->fieldListMap();
            if( !flm[ "ALBUMARTIST" ].isEmpty() )
                m_data.albumArtist = strip( flm[ "ALBUMARTIST" ].front() );
            if( !flm[ "COMPOSER" ].isEmpty() )
                m_data.composer = strip( flm[ "COMPOSER" ].front() );
            if( !flm[ "DISCNUMBER" ].isEmpty() )
                disc = strip( flm[ "DISCNUMBER" ].front() );
            if( !flm[ "BPM" ].isEmpty() )
                m_data.bpm = TStringToQString( flm[ "BPM" ].front() ).toFloat();
        }
    }

    else if( TagLib::FLAC::File *file = dynamic_cast< TagLib::FLAC::File *>( fileRef.file() ) )
    {
        if( file->xiphComment() )
        {
            const TagLib::Ogg::FieldListMap flm = file->xiphComment()->fieldListMap();
            if( !flm[ "ALBUMARTIST" ].isEmpty() )
                m_data.albumArtist = strip( flm[ "ALBUMARTIST" ].front() );
            if( !flm[ "COMPOSER" ].isEmpty() )
                m_data.composer = strip( flm[ "COMPOSER" ].front() );
            if( !flm[ "DISCNUMBER" ].isEmpty() )
                disc = strip( flm[ "DISCNUMBER" ].front() );
            if( !flm[ "BPM" ].isEmpty() )
                m_data.bpm = TStringToQString( flm[ "BPM" ].front() ).toFloat();
        }
    }
    else if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileRef.file() ) )
    {
        TagLib::MP4::Tag *mp4tag = dynamic_cast< TagLib::MP4::Tag *>( file->tag() );
        if( mp4tag )
        {
            if ( mp4tag->itemListMap().contains( "aART" ) )
                m_data.albumArtist = strip( mp4tag->itemListMap()["aART"].toStringList().front() );

            if ( mp4tag->itemListMap().contains( "\xA9wrt" ) )
                m_data.composer = strip( mp4tag->itemListMap()["\xA9wrt"].toStringList().front() );

            if ( mp4tag->itemListMap().contains( "disk" ) )
                disc = QString::number( mp4tag->itemListMap()["disk"].toIntPair().first );

            if ( mp4tag->itemListMap().contains( "tmpo" ) )
                m_data.bpm = mp4tag->itemListMap()["tmpo"].toIntPair().first;
        }
    }
    else if( TagLib::MPC::File *file = dynamic_cast< TagLib::MPC::File *>( fileRef.file() ) )
    {
        if( file->APETag() )
        {
            const TagLib::APE::ItemListMap &itemsMap = file->APETag()->itemListMap();
            if( itemsMap.contains( "Album Artist" ) )
                m_data.albumArtist = strip( itemsMap[ "Album Artist" ].toString() );
            if( itemsMap.contains( "Composer" ) )
                m_data.composer = strip( itemsMap[ "Composer" ].toString() );
            if( itemsMap.contains( "Disc" ) )
                disc = strip( itemsMap[ "Disc" ].toString() );
            if( itemsMap.contains( "BPM" ) )
                m_data.bpm = TStringToQString( itemsMap[ "BPM" ].toString() ).toFloat();
        }
    }
    if( !disc.isEmpty() )
    {
        int i = disc.indexOf( '/' );
        if( i != -1 )
            m_data.discNumber = disc.left( i ).toInt();
        else
            m_data.discNumber = disc.toInt();
    }
#undef strip
    if(url.isLocalFile())
    {
        m_data.fileSize = QFile( url.toLocalFile() ).size();
    }
    else
    {
        m_data.fileSize = QFile( url.path() ).size();
    }

    //as a last ditch effort, use the filename as the title if nothing else has been found
    if ( m_data.title.isEmpty() )
    {
        m_data.title = url.fileName();
    }

    /* we can't do this. there is a difference between a track artist and an album artist.
       also the auto test breaks.
    if( m_data.artist.isEmpty() && !m_data.albumArtist.isEmpty() )
        m_data.artist = m_data.albumArtist;
    else if( !m_data.artist.isEmpty() && m_data.albumArtist.isEmpty() )
        m_data.albumArtist = m_data.artist;
        */

    // debug() << "Read metadata from file for: " + m_data.title;
}

// internal helper classes

class FileArtist : public Meta::Artist
{
public:
    FileArtist( MetaFile::Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        const QString artist = d.data()->m_data.artist;
        return artist;
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Artist &other ) const {
        return name() == other.name();
    }

    QWeakPointer<MetaFile::Track::Private> const d;
};

class FileAlbum : public Meta::Album
{
public:
    FileAlbum( MetaFile::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const
    {
        return false;
    }

    bool hasAlbumArtist() const
    {
        return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
        {
            const QString albumName = d.data()->m_data.album;
            return albumName;
        }
        else
            return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image( int size )
    {
        return Meta::Album::image( size );
    }

    bool operator==( const Meta::Album &other ) const {
        return name() == other.name();
    }

    QWeakPointer<MetaFile::Track::Private> const d;
};

class FileGenre : public Meta::Genre
{
public:
    FileGenre( MetaFile::Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        const QString genreName = d.data()->m_data.genre;
        return genreName;
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Genre &other ) const {
        return name() == other.name();
    }

    QWeakPointer<MetaFile::Track::Private> const d;
};

class FileComposer : public Meta::Composer
{
public:
    FileComposer( MetaFile::Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        const QString composer = d.data()->m_data.composer;
        return composer;
     }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Composer &other ) const {
        return name() == other.name();
    }

    QWeakPointer<MetaFile::Track::Private> const d;
};

class FileYear : public Meta::Year
{
public:
    FileYear( MetaFile::Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        const QString year = QString::number( d.data()->m_data.year );
        return year;
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Year &other ) const {
        return name() == other.name();
    }

    QWeakPointer<MetaFile::Track::Private> const d;
};


}

#endif
