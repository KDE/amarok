//
// C++ Interface: sqlmeta
//
// Description: Implementation of the Meta namespace for Amarok's local SQL collcetion
//
//
// Author: Maximilian Kossick <maximilian.kossick@googlemail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "meta.h"

#include <QMutex>
#include <QStringList>

using namespace Meta;

class SqlTrack : public Track
{
    public:
        SqlTrack( const QStringList &queryResult );

        virtual QString name() const { return m_title; }
        virtual QString prettyTitle() const;

        virtual KUrl playableUrl() const { return m_url; }
        virtual QString prettyUrl() const { return m_url.path(); }

        virtual bool isPlayable() const;
        virtual bool isEditable() const;

        virtual AlbumPtr album() const { return m_album; }
        virtual void setArtist( const QString &newArtist );
        virtual ArtistPtr artist() const { return m_artist; }
        virtual ComposerPtr composer() const { return m_composer; }
        virtual GenrePtr genre() const { return m_genre; }

        virtual QString type() const;

        //helper functions
        static QString prettyTitle( const QString &filename );

    private:

        QString m_title;
        KUrl m_url;

        int m_length;
        int m_filesize;
        int m_trackNumber;
        int m_discNumber;
        uint m_lastPlayed;
        int m_playCount;
        int m_bitrate;
        int m_sampleRate;
        int m_rating;
        double m_score;

        AlbumPtr m_album;
        ArtistPtr m_artist;
        GenrePtr m_genre;
        ComposerPtr m_composer;
};

class SqlArtist : public Artist
{
    public:
        SqlArtist( const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; } //change if necessary

        virtual void invalidateCache();

        virtual TrackList tracks();

    private:
        QString m_name;
        bool m_tracksLoaded;
        TrackList m_tracks;
        //QReadWriteLock doesnt support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

};
