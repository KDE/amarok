/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_STREAMITEM_SCRIPT_H
#define AMAROK_STREAMITEM_SCRIPT_H

#include <QMetaType> // for Q_DECLARE_METATYPE
#include <QObject>

class QScriptEngine;

//namespace AmarokScript{

    // SCRIPTDOX: Amarok.StreamItem
    class StreamItem : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( QString itemName WRITE setItemName READ itemName )
        Q_PROPERTY( QString infoHtml WRITE setInfoHtml READ infoHtml )
        Q_PROPERTY( QString playableUrl WRITE setPlayableUrl READ playableUrl )
        Q_PROPERTY( QString callbackData WRITE setCallbackData READ callbackData )
        Q_PROPERTY( int level WRITE setLevel READ level )

        Q_PROPERTY( QString album WRITE setAlbum READ album )
        Q_PROPERTY( QString artist WRITE setArtist READ artist )
        Q_PROPERTY( QString genre WRITE setGenre READ genre )
        Q_PROPERTY( QString composer WRITE setComposer READ composer )
        Q_PROPERTY( int year WRITE setYear READ year )
        Q_PROPERTY( QString coverUrl WRITE setCoverUrl READ coverUrl )

        public:
            explicit StreamItem( QScriptEngine *engine );

            QString itemName() const;
            QString infoHtml() const;
            QString playableUrl() const;
            QString callbackData() const;
            int level() const;

            QString album() const;
            QString artist() const;
            QString genre() const;
            QString composer() const;
            int year() const;
            QString coverUrl();

        private:
            void setItemName( const QString &name );
            void setInfoHtml( const QString &infoHtml );
            void setPlayableUrl( const QString &playableUrl );
            void setCallbackData( const QString &callbackData );
            void setLevel( int level );

            void setAlbum( const QString &album );
            void setArtist( const QString &artist );
            void setGenre( const QString &genre );
            void setComposer( const QString &composer );
            void setYear( int year );
            void setCoverUrl( const QString &url );

            QString m_name;
            QString m_infoHtml;
            QString m_playableUrl;
            QString m_callbackData;
            int m_level;

            //these are not required but can be used to override what is shown in the playlist and elsewhere.
            QString m_album;
            QString m_artist;
            QString m_genre;
            QString m_composer;
            int m_year;
            QString m_coverUrl;

    };
//}

Q_DECLARE_METATYPE( StreamItem* )

#endif // AMAROK_STREAMITEM_SCRIPT_H
