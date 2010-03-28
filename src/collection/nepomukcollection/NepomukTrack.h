/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#ifndef NEPOMUKTRACK_H
#define NEPOMUKTRACK_H

#include "core/meta/Meta.h"

#include <QDateTime>

#include <Nepomuk/Resource>
#include <Soprano/BindingSet>
#include <Soprano/Model>
#include <Soprano/LiteralValue>


class NepomukCollection;
class NepomukRegistry;

namespace Meta
{

class NepomukTrack;
typedef KSharedPtr<NepomukTrack> NepomukTrackPtr;

class WriteStatisticsThread;


class NepomukTrack : public Track
{
    public:
        NepomukTrack( NepomukCollection *collection, NepomukRegistry *registry, const Soprano::BindingSet &data );
        ~NepomukTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString uidUrl() const;
        virtual QString prettyUrl() const;

        virtual bool isPlayable() const;
        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        virtual QString comment() const;

        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual qint64 length() const;

        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;

        virtual int trackNumber() const;

        virtual int discNumber() const;

        virtual uint firstPlayed() const;
        virtual uint lastPlayed() const;
        virtual int playCount() const;

        virtual QString type() const;

        virtual void finishedPlaying( double playedFraction );
        virtual void setCachedLyrics ( const QString& value );
        virtual QString cachedLyrics() const;

    // for use in nepomuk plugin only

        void writeStatistics( void );
        QUrl resourceUri() const;
        void valueChangedInNepomuk( qint64 value, const Soprano::LiteralValue& );

        void setUid ( const QString& value );
        QString uid() const;

         Nepomuk::Resource& resource();
         void setResource ( const Nepomuk::Resource& value );

    private:
        NepomukCollection *m_collection;
        Nepomuk::Resource m_nepores;
        KUrl m_url;
        QString m_title;
        QString m_artist;
        QString m_album;
        QString m_genre;
        QString m_year;
        QString m_composer;
        QString m_type;
        QString m_comment;
        int m_trackNumber;
        int m_length;
        int m_rating;
        int m_bitrate;
        int m_discNumber;
        int m_filesize;
        int m_playCount;
        int m_sampleRate;
        int m_score;
        QDateTime m_createDate;
        QDateTime m_firstPlayed;
        QDateTime m_lastPlayed;
        NepomukRegistry *m_registry;
        QTime m_lastWrote;
        QString m_uid;
};

}
#endif /*NEPOMUKTRACK_H*/
