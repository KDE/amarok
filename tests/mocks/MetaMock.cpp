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

#include "MetaMock.h"

#include "meta/MetaConstants.h"

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

    QString m_name;
};


MetaMock::MetaMock( const QVariantMap &data )
    : Meta::Track()
    , m_data( data )
{
}

MetaMock::~MetaMock()
{
}
