/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef METAMOCK_H
#define METAMOCK_H

#include "meta/Meta.h"
#include "meta/MetaConstants.h"

#include <QVariantMap>

/**
  * This class provides simple mocks for meta classes.
  * it will look for the keys defined in meta/MetaConstants.h
  * in the given QVariantMap and return those values in the respective methods.
  */
class MetaMock : public Meta::Track
{
public:
    MetaMock( const QVariantMap &data ) : Meta::Track(), m_data( data ) {}
    virtual ~MetaMock() {}

    Meta::AlbumPtr album() const { return m_album; }
    Meta::YearPtr year() const { return m_year; }
    Meta::GenrePtr genre() const { return m_genre; }
    Meta::ArtistPtr artist() const { return m_artist; }
    Meta::ComposerPtr composer() const { return m_composer; }


    QString name() const { return m_data.value( Meta::Field::TITLE ).toString(); }
    QString prettyName() const { return name(); }
    KUrl playableUrl() const { return m_data.value( Meta::Field::URL ).value<KUrl>(); }
    QString prettyUrl() const { return playableUrl().url(); }
    QString uidUrl() const { return m_data.value( Meta::Field::UNIQUEID ).toString(); }
    bool isPlayable() const { return false; }
    QString comment() const { return m_data.value( Meta::Field::COMMENT ).toString(); }
    float bpm() const { return m_data.value( Meta::Field::BPM ).toDouble(); }
    qint64 length() const { return m_data.value( Meta::Field::LENGTH ).toInt(); }
    int filesize() const { return m_data.value( Meta::Field::FILESIZE ).toInt(); }
    int sampleRate() const { return m_data.value( Meta::Field::SAMPLERATE ).toInt(); }
    int bitrate() const { return m_data.value( Meta::Field::BITRATE ).toInt(); }
    QDateTime createDate() const { return QDateTime(); }    //field missing
    int trackNumber() const { return m_data.value( Meta::Field::TRACKNUMBER ).toInt(); }
    int discNumber() const { return m_data.value( Meta::Field::DISCNUMBER ).toInt(); }
    uint firstPlayed() const { return m_data.value( Meta::Field::FIRST_PLAYED ).toDateTime().toTime_t(); }
    uint lastPlayed() const { return m_data.value( Meta::Field::LAST_PLAYED ).toDateTime().toTime_t(); }
    int playCount() const { return m_data.value( Meta::Field::PLAYCOUNT ).toInt(); }
    QString type() const { return "Mock"; }
    double score() const { return m_data.value( Meta::Field::SCORE ).toDouble(); }
    void setScore( double newScore ) { Q_UNUSED( newScore ); }
    int rating() const { return m_data.value( Meta::Field::RATING ).toInt(); }
    void setRating( int newRating ) { Q_UNUSED( newRating ); }

public:
    QVariantMap m_data;
    Meta::ArtistPtr m_artist;
    Meta::AlbumPtr m_album;
    Meta::GenrePtr m_genre;
    Meta::YearPtr m_year;
    Meta::ComposerPtr m_composer;
};

class MockYear : public Meta::Year
{
public:
    MockYear( const QString &name )
        : Meta::Year()
        , m_name( name ) {}

    QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    Meta::TrackList tracks() { return Meta::TrackList(); }

    QString m_name;
};

class MockGenre : public Meta::Genre
{
public:
    MockGenre( const QString &name )
        : Meta::Genre()
        , m_name( name ) {}

    QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    Meta::TrackList tracks() { return Meta::TrackList(); }

    QString m_name;
};

class MockComposer : public Meta::Composer
{
public:
    MockComposer( const QString &name )
        : Meta::Composer()
        , m_name( name ) {}

    QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    Meta::TrackList tracks() { return Meta::TrackList(); }

    QString m_name;
};

class MockArtist : public Meta::Artist
{
public:
    MockArtist( const QString &name )
        : Meta::Artist()
        , m_name( name ) {}

    QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    Meta::TrackList tracks() { return Meta::TrackList(); }
    Meta::AlbumList albums() { return Meta::AlbumList(); }

    QString m_name;
};

class MockAlbum : public Meta::Album
{
public:
    MockAlbum( const QString &name )
        : Meta::Album()
        , m_name( name ) {}

    QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    Meta::TrackList tracks() { return Meta::TrackList(); }
    bool hasAlbumArtist() const { return false; }
    Meta::ArtistPtr albumArtist() const { return Meta::ArtistPtr(); }
    bool isCompilation() const { return !hasAlbumArtist(); }

    QString m_name;
};

#endif // METAMOCK_H
