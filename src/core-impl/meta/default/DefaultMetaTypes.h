/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DEFAULTMETATYPES_H
#define DEFAULTMETATYPES_H

#include "amarok_export.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <KLocalizedString>


namespace Meta
{

class AMAROK_EXPORT DefaultArtist : public Meta::Artist
{
    public:

        DefaultArtist() {}
        virtual ~DefaultArtist() {}

        QString name() const override { return i18nc( "The value is not known", "Unknown" ); }

        TrackList tracks() override { return TrackList(); }
};

class AMAROK_EXPORT DefaultAlbum : public Meta::Album
{
    public:

        DefaultAlbum()
            : Meta::Album()
            , m_albumArtist( new DefaultArtist() ) {}
        virtual ~DefaultAlbum() {}

        bool hasAlbumArtist() const override { return true; }
        ArtistPtr albumArtist() const override { return m_albumArtist; }

        bool isCompilation() const override { return false; }

        QString name() const override { return i18nc( "The Value is not known", "Unknown" ); }

        TrackList tracks() override { return TrackList(); }

    private:
        Meta::ArtistPtr m_albumArtist;

};


class AMAROK_EXPORT DefaultComposer : public Meta::Composer
{
    public:

        DefaultComposer() {}
        virtual ~DefaultComposer() {}

        QString name() const override { return i18nc( "The value is not known", "Unknown" ); }

        TrackList tracks() override { return TrackList(); }

    private:

        static ComposerPtr s_instance;

};

class AMAROK_EXPORT DefaultGenre : public Meta::Genre
{
    public:

        DefaultGenre() {}
        virtual ~DefaultGenre() {}

        QString name() const override { return i18nc( "The value is not known", "Unknown" ); }

        TrackList tracks() override { return TrackList(); }

};
class AMAROK_EXPORT DefaultYear : public Meta::Year
{
    public:

        DefaultYear() {}
        virtual ~DefaultYear() {}

        QString name() const override { return QStringLiteral("0"); }

        TrackList tracks() override { return TrackList(); }

};

}

#endif


