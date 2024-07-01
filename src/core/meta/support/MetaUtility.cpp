/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "core/meta/support/MetaUtility.h"

#include "core/capabilities/Capability.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core/support/Debug.h"

#include <QTime>
#include <QFile>

#include <KIO/Global>
#include <KLocalizedString>

        static const QString XESAM_ALBUM          = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#album");
        static const QString XESAM_ALBUMARTIST    = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#albumArtist");
        static const QString XESAM_ARTIST         = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#artist");
        static const QString XESAM_BITRATE        = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#audioBitrate");
        static const QString XESAM_BPM            = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#audioBPM");
        static const QString XESAM_CODEC          = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#audioCodec");
        static const QString XESAM_COMMENT        = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#comment");
        static const QString XESAM_COMPOSER       = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#composer");
        static const QString XESAM_DISCNUMBER     = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#discNumber");
        static const QString XESAM_FILESIZE       = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#size");
        static const QString XESAM_GENRE          = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#genre");
        static const QString XESAM_LENGTH         = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#mediaDuration");
        static const QString XESAM_RATING         = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#userRating");
        static const QString XESAM_SAMPLERATE     = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#audioSampleRate");
        static const QString XESAM_TITLE          = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#title");
        static const QString XESAM_TRACKNUMBER    = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#trackNumber");
        static const QString XESAM_URL            = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#url");
        static const QString XESAM_YEAR           = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#contentCreated");

        static const QString XESAM_SCORE          = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#autoRating");
        static const QString XESAM_PLAYCOUNT      = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#useCount");
        static const QString XESAM_FIRST_PLAYED   = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#firstUsed");
        static const QString XESAM_LAST_PLAYED    = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#lastUsed");

        static const QString XESAM_ID             = QStringLiteral("http://freedesktop.org/standards/xesam/1.0/core#id");
        //static bool conversionMapsInitialised = false;

QVariantMap
Meta::Field::mapFromTrack( const Meta::TrackPtr &track )
{
    QVariantMap map;
    if( !track )
        return map;

    if( track->name().isEmpty() )
        map.insert( Meta::Field::TITLE, QVariant( track->prettyName() ) );
    else
        map.insert( Meta::Field::TITLE, QVariant( track->name() ) );
    if( track->artist() && !track->artist()->name().isEmpty() )
        map.insert( Meta::Field::ARTIST, QVariant( track->artist()->name() ) );
    if( track->album() && !track->album()->name().isEmpty() )
    {
        map.insert( Meta::Field::ALBUM, QVariant( track->album()->name() ) );
        if( track->album()->hasAlbumArtist() && !track->album()->albumArtist()->name().isEmpty() )
            map.insert( Meta::Field::ALBUMARTIST, QVariant( track->album()->albumArtist()->name() ) );
    }
    if( track->filesize() )
        map.insert( Meta::Field::FILESIZE, QVariant( track->filesize() ) );
    if( track->genre() && !track->genre()->name().isEmpty() )
        map.insert( Meta::Field::GENRE, QVariant( track->genre()->name() ) );
    if( track->composer() && !track->composer()->name().isEmpty() )
        map.insert( Meta::Field::COMPOSER, QVariant( track->composer()->name() ) );
    if( track->year() && !track->year()->name().isEmpty() )
        map.insert( Meta::Field::YEAR, QVariant( track->year()->name() ) );
    if( !track->comment().isEmpty() )
        map.insert( Meta::Field::COMMENT, QVariant( track->comment() ) );
    if( track->trackNumber() )
        map.insert( Meta::Field::TRACKNUMBER, QVariant( track->trackNumber() ) );
    if( track->discNumber() )
        map.insert( Meta::Field::DISCNUMBER, QVariant( track->discNumber() ) );
    if( track->bitrate() )
        map.insert( Meta::Field::BITRATE, QVariant( track->bitrate() ) );
    if( track->length() )
        map.insert( Meta::Field::LENGTH, QVariant( track->length() ) );
    if( track->sampleRate() )
        map.insert( Meta::Field::SAMPLERATE, QVariant( track->sampleRate() ) );
    if( track->bpm() >= 0.0)
        map.insert( Meta::Field::BPM, QVariant( track->bpm() ) );

    map.insert( Meta::Field::UNIQUEID, QVariant( track->uidUrl() ) );
    map.insert( Meta::Field::URL, QVariant( track->prettyUrl() ) );
    Meta::ConstStatisticsPtr statistics = track->statistics();
    map.insert( Meta::Field::RATING, QVariant( statistics->rating() ) );
    map.insert( Meta::Field::SCORE, QVariant( statistics->score() ) );
    map.insert( Meta::Field::PLAYCOUNT, QVariant( statistics->playCount() ) );
    map.insert( Meta::Field::LAST_PLAYED, QVariant( statistics->lastPlayed() ) );
    map.insert( Meta::Field::FIRST_PLAYED, QVariant( statistics->firstPlayed() ) );

    return map;
}

QVariantMap
Meta::Field::mprisMapFromTrack( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    QVariantMap map;
    if( track )
    {
        // MANDATORY:
        map[QStringLiteral("location")] = track->playableUrl().url();
        // INFORMATIONAL:
        map[QStringLiteral("title")] = track->prettyName();

        if( track->artist() )
            map[QStringLiteral("artist")] = track->artist()->name();

        if( track->album() )
        {
            map[QStringLiteral("album")] = track->album()->name();
            if( track->album()->hasAlbumArtist() && !track->album()->albumArtist()->name().isEmpty() )
                map[ QStringLiteral("albumartist") ] = track->album()->albumArtist()->name();

            QUrl url = track->album()->imageLocation();
            if ( url.isValid() && !url.isLocalFile() )
            {
                // embedded id?  Request a version to be put in the cache
                QImage image = track->album()->image();
                int width = track->album()->image().width();
                url = track->album()->imageLocation( width );
                debug() << "MPRIS: New location for width" << width << "is" << url;
            }
            if ( url.isValid() && url.isLocalFile() )
                map[QStringLiteral("arturl")] = QString::fromLatin1( url.toEncoded() );
        }

        map[QStringLiteral("tracknumber")] = track->trackNumber();
        map[QStringLiteral("time")] = track->length() / 1000;
        map[QStringLiteral("mtime")] = track->length();

        if( track->genre() )
            map[QStringLiteral("genre")] = track->genre()->name();

        map[QStringLiteral("comment")] = track->comment();
        map[QStringLiteral("rating")] = track->statistics()->rating()/2;  //out of 5, not 10.

        if( track->year() )
            map[QStringLiteral("year")] = track->year()->name();

        //TODO: external service meta info

        // TECHNICAL:
        map[QStringLiteral("audio-bitrate")] = track->bitrate();
        map[QStringLiteral("audio-samplerate")] = track->sampleRate();
        //amarok has no video-bitrate

        // EXTRA Amarok specific
        const QString lyrics = track->cachedLyrics();
        if( !lyrics.isEmpty() )
            map[QStringLiteral("lyrics")] = lyrics;
    }
    return map;
}

QVariantMap
Meta::Field::mpris20MapFromTrack( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    QVariantMap map;
    if( track )
    {
        // We do not set mpris::trackid here because it depends on the position
        // of the track in the playlist
        map[QStringLiteral("mpris:length")] = track->length() * 1000; // microseconds

        // get strong pointers (BR 317980)
        Meta::AlbumPtr    album = track->album();
        Meta::ArtistPtr   artist = track->artist();
        Meta::ComposerPtr composer = track->composer();
        Meta::YearPtr     year = track->year();
        Meta::GenrePtr    genre = track->genre();
        Meta::ConstStatisticsPtr statistics = track->statistics();

        if( album ) {
            QUrl url = album->imageLocation();
            debug() << "MPRIS2: Album image location is" << url;
            if ( url.isValid() && !url.isLocalFile() )
            {
                // embedded id?  Request a version to be put in the cache
                const QImage image = album->image();
                int width = image.width();
                url = album->imageLocation( width );
                debug() << "MPRIS2: New location for width" << width << "is" << url;
            }
            if ( url.isValid() && url.isLocalFile() )
                map[QStringLiteral("mpris:artUrl")] = QString::fromLatin1( url.toEncoded() );

            map[QStringLiteral("xesam:album")] = album->name();
            if ( album->hasAlbumArtist() )
                map[QStringLiteral("xesam:albumArtist")] = QStringList() << album->albumArtist()->name();
        }

        if( artist )
            map[QStringLiteral("xesam:artist")] = QStringList() << artist->name();

        const QString lyrics = track->cachedLyrics();
        if( !lyrics.isEmpty() )
            map[QStringLiteral("xesam:asText")] = lyrics;

        if( track->bpm() > 0 )
            map[QStringLiteral("xesam:audioBPM")] = int(track->bpm());

        map[QStringLiteral("xesam:autoRating")] = statistics->score();

        map[QStringLiteral("xesam:comment")] = QStringList() << track->comment();

        if( composer )
            map[QStringLiteral("xesam:composer")] = QStringList() << composer->name();

        if( year )
            map[QStringLiteral("xesam:contentCreated")] = QDate(year->year(), 1, 1).startOfDay().toString(Qt::ISODate);

        if( track->discNumber() )
            map[QStringLiteral("xesam:discNumber")] = track->discNumber();

        if( statistics->firstPlayed().isValid() )
            map[QStringLiteral("xesam:firstUsed")] = statistics->firstPlayed().toString(Qt::ISODate);

        if( genre )
            map[QStringLiteral("xesam:genre")] = QStringList() << genre->name();

        if( statistics->lastPlayed().isValid() )
            map[QStringLiteral("xesam:lastUsed")] = statistics->lastPlayed().toString(Qt::ISODate);

        map[QStringLiteral("xesam:title")] = track->prettyName();

        map[QStringLiteral("xesam:trackNumber")] = track->trackNumber();

        map[QStringLiteral("xesam:url")] = track->playableUrl().url();

        map[QStringLiteral("xesam:useCount")] = statistics->playCount();

        map[QStringLiteral("xesam:userRating")] = statistics->rating() / 10.; // xesam:userRating is a float
    }
    return map;
}


void
Meta::Field::updateTrack( Meta::TrackPtr track, const QVariantMap &metadata )
{
    if( !track )
        return;
    Meta::TrackEditorPtr ec = track->editor();
    if( !ec )
        return;

    ec->beginUpdate();
    QString title = metadata.contains( Meta::Field::TITLE ) ?
                            metadata.value( Meta::Field::TITLE ).toString() : QString();
    ec->setTitle( title );
    QString comment = metadata.contains( Meta::Field::COMMENT ) ?
                            metadata.value( Meta::Field::COMMENT ).toString() : QString();
    ec->setComment( comment );
    int tracknr = metadata.contains( Meta::Field::TRACKNUMBER ) ?
                            metadata.value( Meta::Field::TRACKNUMBER ).toInt() : 0;
    ec->setTrackNumber( tracknr );
    int discnr = metadata.contains( Meta::Field::DISCNUMBER ) ?
                            metadata.value( Meta::Field::DISCNUMBER ).toInt() : 0;
    ec->setDiscNumber( discnr );
    QString artist = metadata.contains( Meta::Field::ARTIST ) ?
                            metadata.value( Meta::Field::ARTIST ).toString() : QString();
    ec->setArtist( artist );
    QString album = metadata.contains( Meta::Field::ALBUM ) ?
                            metadata.value( Meta::Field::ALBUM ).toString() : QString();
    ec->setAlbum( album );
    QString albumArtist = metadata.contains( Meta::Field::ALBUMARTIST ) ?
                            metadata.value( Meta::Field::ALBUMARTIST ).toString() : QString();
    ec->setAlbumArtist(albumArtist);
    QString genre = metadata.contains( Meta::Field::GENRE ) ?
                            metadata.value( Meta::Field::GENRE ).toString() : QString();
    ec->setGenre( genre );
    QString composer = metadata.contains( Meta::Field::COMPOSER ) ?
                            metadata.value( Meta::Field::COMPOSER ).toString() : QString();
    ec->setComposer( composer );
    int year = metadata.contains( Meta::Field::YEAR ) ?
                            metadata.value( Meta::Field::YEAR ).toInt() : 0;
    ec->setYear( year );
    ec->endUpdate();
}

QString
Meta::Field::xesamPrettyToFullFieldName( const QString &name )
{
    if( name == Meta::Field::ARTIST )
        return XESAM_ARTIST;
    else if( name == Meta::Field::ALBUM )
        return XESAM_ALBUM;
    else if( name == Meta::Field::ALBUMARTIST )
        return XESAM_ALBUMARTIST;
    else if( name == Meta::Field::BITRATE )
        return XESAM_BITRATE;
    else if( name == Meta::Field::BPM )
        return XESAM_BPM;
    else if( name == Meta::Field::CODEC )
        return XESAM_CODEC;
    else if( name == Meta::Field::COMMENT )
        return XESAM_COMMENT;
    else if( name == Meta::Field::COMPOSER )
        return XESAM_COMPOSER;
    else if( name == Meta::Field::DISCNUMBER )
        return XESAM_DISCNUMBER;
    else if( name == Meta::Field::FILESIZE )
        return XESAM_FILESIZE;
    else if( name == Meta::Field::GENRE )
        return XESAM_GENRE;
    else if( name == Meta::Field::LENGTH )
        return XESAM_LENGTH;
    else if( name == Meta::Field::RATING )
        return XESAM_RATING;
    else if( name == Meta::Field::SAMPLERATE )
        return XESAM_SAMPLERATE;
    else if( name == Meta::Field::TITLE )
        return XESAM_TITLE;
    else if( name == Meta::Field::TRACKNUMBER )
        return XESAM_TRACKNUMBER;
    else if( name == Meta::Field::URL )
        return XESAM_URL;
    else if( name == Meta::Field::YEAR )
        return XESAM_YEAR;
    else if( name==Meta::Field::SCORE )
        return XESAM_SCORE;
    else if( name==Meta::Field::PLAYCOUNT )
        return XESAM_PLAYCOUNT;
    else if( name==Meta::Field::FIRST_PLAYED )
        return XESAM_FIRST_PLAYED;
    else if( name==Meta::Field::LAST_PLAYED )
        return XESAM_LAST_PLAYED;
    else if( name==Meta::Field::UNIQUEID )
        return XESAM_ID;
    else
        return QStringLiteral("xesamPrettyToFullName: unknown name ") + name;
}

QString
Meta::Field::xesamFullToPrettyFieldName( const QString &name )
{
    if( name == XESAM_ARTIST )
        return Meta::Field::ARTIST;
    if( name == XESAM_ALBUMARTIST )
        return Meta::Field::ALBUMARTIST;
    else if( name == XESAM_ALBUM )
        return Meta::Field::ALBUM;
    else if( name == XESAM_BITRATE )
        return Meta::Field::BITRATE;
    else if( name == XESAM_BPM )
        return Meta::Field::BPM;
    else if( name == XESAM_CODEC )
        return Meta::Field::CODEC;
    else if( name == XESAM_COMMENT )
        return Meta::Field::COMMENT;
    else if( name == XESAM_COMPOSER )
        return Meta::Field::COMPOSER;
    else if( name == XESAM_DISCNUMBER )
        return Meta::Field::DISCNUMBER;
    else if( name == XESAM_FILESIZE )
        return Meta::Field::FILESIZE;
    else if( name == XESAM_GENRE )
        return Meta::Field::GENRE;
    else if( name == XESAM_LENGTH )
        return Meta::Field::LENGTH;
    else if( name == XESAM_RATING )
        return Meta::Field::RATING;
    else if( name == XESAM_SAMPLERATE )
        return Meta::Field::SAMPLERATE;
    else if( name == XESAM_TITLE )
        return Meta::Field::TITLE;
    else if( name == XESAM_TRACKNUMBER )
        return Meta::Field::TRACKNUMBER;
    else if( name == XESAM_URL )
        return Meta::Field::URL;
    else if( name == XESAM_YEAR )
        return Meta::Field::YEAR;
    else if( name == XESAM_SCORE )
        return Meta::Field::SCORE;
    else if( name == XESAM_PLAYCOUNT )
        return Meta::Field::PLAYCOUNT;
    else if( name == XESAM_FIRST_PLAYED )
        return Meta::Field::FIRST_PLAYED;
    else if( name == XESAM_LAST_PLAYED )
        return Meta::Field::LAST_PLAYED;
    else if( name == XESAM_ID )
        return Meta::Field::UNIQUEID;
    else
        return QStringLiteral("xesamFullToPrettyName: unknown name ") + name;
}


QString
Meta::msToPrettyTime( qint64 ms )
{
    return Meta::secToPrettyTime( ms / 1000 );
}

QString
Meta::secToPrettyTime( int seconds )
{
    if( seconds < 60 * 60 ) // one hour
        return QTime(0, 0, 0).addSecs( seconds ).toString( i18nc("the time format for a time length when the time is below 1 hour see QTime documentation.", "m:ss" ) );
    // split days off for manual formatting (QTime doesn't work properly > 1 day,
    // QDateTime isn't suitable as it thinks it's a date)
    int days = seconds / 86400;
    seconds %= 86400;
    QString reply = QLatin1String("");
    if ( days > 0 )
        reply += i18ncp("number of days with spacing for the pretty time", "%1 day, ", "%1 days, ", days);
    reply += QTime(0, 0, 0).addSecs( seconds ).toString( i18nc("the time format for a time length when the time is 1 hour or above see QTime documentation.", "h:mm:ss" ) );
    return reply;
}

QString
Meta::secToPrettyTimeLong( int seconds )
{
    int minutes = seconds / 60;
    int hours = minutes / 60;
    int days = hours / 24;
    int months = days / 30; // a short month
    int years = months / 12;

    if( months > 24 || (((months % 12) == 0) && years > 0) )
        return i18ncp("number of years for the pretty time", "%1 year", "%1 years", years);
    if( days > 60 || (((days % 30) == 0) && months > 0) )
        return i18ncp("number of months for the pretty time", "%1 month", "%1 months", months);
    if( hours > 24  || (((hours % 24) == 0) && days > 0) )
        return i18ncp("number of days for the pretty time", "%1 day", "%1 days", days);
    if( minutes > 120 || (((minutes % 60) == 0) && hours > 0) )
        return i18ncp("number of hours for the pretty time", "%1 hour", "%1 hours", hours);
    if( seconds > 120 || (((seconds % 60) == 0) && minutes > 0) )
        return i18ncp("number of minutes for the pretty time", "%1 minute", "%1 minutes", hours);

    return i18ncp("number of seconds for the pretty time", "%1 second", "%1 seconds", hours);
}

QString
Meta::prettyFilesize( quint64 size )
{
    return KIO::convertSize( size );
}

QString
Meta::prettyBitrate( int bitrate )
{
    //the point here is to force sharing of these strings returned from prettyBitrate()
    static const QString bitrateStore[9] = {
        QStringLiteral("?"), QStringLiteral("32"), QStringLiteral("64"), QStringLiteral("96"),
        QStringLiteral("128"), QStringLiteral("160"), QStringLiteral("192"), QStringLiteral("224"), QStringLiteral("256") };

    return (bitrate >=0 && bitrate <= 256 && bitrate % 32 == 0)
                ? bitrateStore[ bitrate / 32 ]
    : QStringLiteral( "%1" ).arg( bitrate );
}
