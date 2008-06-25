/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef NEPOMUKMETA_H_
#define NEPOMUKMETA_H_

#include "NepomukCollection.h"

#include "Meta.h"

#include <QDateTime>
#include <QMutex>

#include <Nepomuk/Resource>
#include <Soprano/BindingSet>
#include <Soprano/Model>


namespace Meta
{

class NepomukTrack;
class NepomukAlbum;
class NepomukArtist;
class NepomukGenre;
class NepomukComposer;
class NepomukYear;

typedef KSharedPtr<NepomukTrack> NepomukTrackPtr;
typedef KSharedPtr<NepomukArtist> NepomukArtistPtr;
typedef KSharedPtr<NepomukAlbum> NepomukAlbumPtr;
typedef KSharedPtr<NepomukGenre> NepomukGenrePtr;
typedef KSharedPtr<NepomukComposer> NepomukComposerPtr;
typedef KSharedPtr<NepomukYear> NepomukYearPtr;

class NepomukArtist : public Artist
{
    public:
        NepomukArtist( const QString &name );
        virtual ~NepomukArtist() {};

        virtual QString name() const;
        virtual QString prettyName() const;
        virtual QString sortableName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

    private:
        QString m_name;
        mutable QString m_sortName;
};

class NepomukAlbum : public Album
{
    public:
        NepomukAlbum( const QString &name, const QString &artist );
        virtual ~NepomukAlbum() {};

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();
        
        virtual bool isCompilation() const;

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;


    private:
        QString m_name;
        QString m_artist;
};

class WriteStatisticsThread;
class NepomukTrack : public Track
    {
    public:
        NepomukTrack( NepomukCollection* collection, const Soprano::BindingSet data );
        ~NepomukTrack();
        
        virtual QString name() const;
        virtual QString prettyName() const;
        
        virtual KUrl playableUrl() const;
        virtual QString url() const;
        virtual QString prettyUrl() const;
        
        virtual bool isPlayable() const;
        virtual bool inCollection() const;
        
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
        
        virtual int length() const;
        
        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;
        
        virtual int trackNumber() const;
        
        virtual int discNumber() const;
        
        virtual uint lastPlayed() const;
        virtual int playCount() const;
        
        virtual QString type() const;
        
        virtual void finishedPlaying( double playedFraction );
        
        // for use in nepomuk plugin only
        
        void writeStatistics( void );
        
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
        WriteStatisticsThread *statsThread;
        QMutex statsMutex;
    };


class NepomukGenre : public Genre
{
    public:
        NepomukGenre( const QString &name );
        
        virtual QString name() const;
        virtual QString prettyName() const;
        
        virtual TrackList tracks();
        
    private:
        QString m_name;
};

class NepomukComposer : public Composer
{
    public:
        NepomukComposer( const QString &name );
        
        virtual QString name() const;
        virtual QString prettyName() const;
        
        virtual TrackList tracks();
        
    private:
        QString m_name;
};

class NepomukYear : public Year
{
    public:
        NepomukYear( const QString &name );
        
        virtual QString name() const;
        virtual QString prettyName() const;
        
        virtual TrackList tracks();
        
    private:
        QString m_name;
};

}
#endif /*NEPOMUKMETA_H_*/
