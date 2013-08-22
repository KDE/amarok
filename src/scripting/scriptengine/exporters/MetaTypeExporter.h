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

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "core/meta/Observer.h"

#include <QObject>
#include <QScriptable>

class QScriptContext;
class QScriptEngine;
class QScriptValue;

namespace AmarokScript
{
    // ANM-TODO: Expose other meta classes, expand query makers
    // SCRIPTDOX PROTOTYPE Meta::TrackPtr Track
    #ifdef DEBUG
        class AMAROK_EXPORT
    #else
        class
    #endif
    MetaTrackPrototype : public QObject, public Meta::Observer
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
        Q_PROPERTY( int year WRITE setYear READ year )
        Q_PROPERTY( QString comment WRITE setComment READ comment )
        Q_PROPERTY( QString path READ path )
        Q_PROPERTY( bool isValid READ isValid )
        Q_PROPERTY( bool isEditable READ isEditable )
        Q_PROPERTY( QString lyrics WRITE setLyrics READ lyrics )
        Q_PROPERTY( QString imageUrl WRITE setImageUrl READ imageUrl )
        Q_PROPERTY( QString url READ url )
        Q_PROPERTY( double bpm READ bpm ) // setter not yet available in Meta::Track

        /**
         * Tracks may be loaded asynchronously, indicates whether the track has been loaded and its full metadata available .
         * Connect to the trackLoaded signal to be notified when it is.
         */
        Q_PROPERTY( bool isLoaded READ isLoaded )

        public:
            static void init( QScriptEngine *engine );
            Meta::TrackPtr data() { return m_track; }
            MetaTrackPrototype( const Meta::TrackPtr &track );
            static QScriptValue trackCtor( QScriptContext* context, QScriptEngine* engine );

        public slots:
            /**
             * Returns the image for the album, usually the cover image, if it has one,
             * or an undefined value otherwise.
             */
            QScriptValue imagePixmap( int size = 1 ) const;

        signals:
            /**
             * Emitted when a track has finished loading.
             */
            void loaded( Meta::TrackPtr );

        private:
            Meta::TrackPtr m_track;

            void metadataChanged( Meta::TrackPtr track ) override;
            void metadataChanged( Meta::ArtistPtr artist ) override {  Q_UNUSED( artist ) }
            void metadataChanged( Meta::AlbumPtr album ) override { Q_UNUSED( album ) }
            void metadataChanged( Meta::GenrePtr genre ) override {  Q_UNUSED( genre ) }
            void metadataChanged( Meta::ComposerPtr composer ) override {  Q_UNUSED( composer ) }
            void metadataChanged( Meta::YearPtr year ) override {  Q_UNUSED( year ) }

            QScriptEngine *m_engine;

            int sampleRate() const;
            int bitrate() const;
            double score() const;
            int rating() const;
            bool inCollection() const;
            QString type() const;
            qint64 length() const;
            int fileSize() const;
            int trackNumber() const;
            int discNumber() const;
            int playCount() const;
            bool playable() const;
            QString album() const;
            QString artist() const;
            QString composer() const;
            QString genre() const;
            int year() const;
            QString comment() const;
            QString path() const;
            bool isValid() const;
            bool isEditable();
            QString lyrics() const;
            QString title() const;
            QString imageUrl() const;
            QString url() const;
            double bpm() const;
            bool isLoaded();

            void setScore( double score );
            void setRating( int rating );
            void setTrackNumber( int number );
            void setDiscNumber( int number );
            void setAlbum( const QString &album );
            void setArtist( const QString &artist );
            void setComposer( const QString &composer );
            void setGenre( const QString &genre );
            void setYear( int year );
            void setComment( const QString &comment );
            void setLyrics( const QString &lyrics );
            void setTitle( const QString& name );
            void setImageUrl( const QString& imageUrl );
    };
}

#endif
