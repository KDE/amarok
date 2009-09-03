/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef METATYPE_EXPORTER_H
#define METATYPE_EXPORTER_H

#include "Meta.h"
#include "EditCapability.h"

#include <QObject>
#include <QScriptable>

#ifdef DEBUG
    class AMAROK_EXPORT
#else
    class
#endif
MetaTrackPrototype : public QObject, protected QScriptable
{
    Q_OBJECT

    Q_PROPERTY( QString title WRITE setTitle READ title )
    Q_PROPERTY( int sampleRate READ sampleRate )
    Q_PROPERTY( int bitrate READ bitrate )
    Q_PROPERTY( double score WRITE setScore READ score )
    Q_PROPERTY( int rating WRITE setRating READ rating )
    Q_PROPERTY( bool inCollection READ inCollection )
    Q_PROPERTY( QString type READ type )
    Q_PROPERTY( int length READ length )
    Q_PROPERTY( int fileSize READ fileSize )
    Q_PROPERTY( int trackNumber WRITE setTrackNumber READ trackNumber )
    Q_PROPERTY( int discNumber WRITE setDiscNumber READ discNumber )
    Q_PROPERTY( int playCount READ playCount )
    Q_PROPERTY( bool playable READ playable )
    Q_PROPERTY( QString album WRITE setAlbum READ album )
    Q_PROPERTY( QString artist WRITE setArtist READ artist )
    Q_PROPERTY( QString composer WRITE setComposer READ composer )
    Q_PROPERTY( QString genre WRITE setGenre READ genre )
    Q_PROPERTY( QString year WRITE setYear READ year )
    Q_PROPERTY( QString comment WRITE setComment READ comment )
    Q_PROPERTY( QString path READ path )
    Q_PROPERTY( bool isValid READ isValid )
    Q_PROPERTY( bool isEditable READ isEditable )
    Q_PROPERTY( QString lyrics WRITE setLyrics READ lyrics )
    Q_PROPERTY( QString imageUrl WRITE setImageUrl READ imageUrl )
    Q_PROPERTY( QString url READ url )

    public:
        MetaTrackPrototype();
        ~MetaTrackPrototype();
    public slots:
        QScriptValue imagePixmap( int size ) const;
        QScriptValue imagePixmap() const;

    private:
        int sampleRate() const;
        int bitrate() const;
        double score() const;
        int rating() const;
        bool inCollection() const;
        QString type() const;
        int length() const;
        int fileSize() const;
        int trackNumber() const;
        int discNumber() const;
        int playCount() const;
        bool playable() const;
        QString album() const;
        QString artist() const;
        QString composer() const;
        QString genre() const;
        QString year() const;
        QString comment() const;
        QString path() const;
        bool isValid() const;
        bool isEditable() const;
        QString lyrics() const;
        QString title() const;
        QString imageUrl() const;
        QString url() const;

        void setScore( double score );
        void setRating( int rating );
        void setTrackNumber( int number );
        void setDiscNumber( int number );
        void setAlbum( QString album );
        void setArtist( QString artist );
        void setComposer( QString composer );
        void setGenre( QString genre );
        void setYear( QString year );
        void setComment( QString comment );
        void setLyrics( QString lyrics );
        void setTitle( const QString& name );
        void setImageUrl( const QString& imageUrl );
};

#endif
