//
// C++ Interface: meta
//
// Description: 
//
//
// Author: Maximilian Kossick <maximilian.kossick@googlemail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef META_H
#define META_H

#include <QList>
#include <QSharedData>
#include <QString>

#include <ksharedptr.h>
#include <kurl.h>

namespace Meta
{
    class Track;
    class Artist;
    class Album;
    class Genre;
    class Composer;

    typedef KSharedPtr<Track> TrackPtr;
    typedef QList<TrackPtr> TrackList;
    typedef KSharedPtr<Artist> ArtistPtr;
    typedef KSharedPtr<Album> AlbumPtr;
    typedef KSharedPtr<Composer> ComposerPtr;
    typedef KSharedPtr<Genre> GenrePtr;

    class MetaBase : public QSharedData
    {
        public:
            virtual QString name() const = 0;
            virtual QString prettyName() const = 0;
    };

    class Track : public MetaBase
    {
        public:

            /** an url which can be played by the engine backends */
            virtual KUrl playableUrl() const = 0;
            /** an url for display purposes */
            virtual QString prettyUrl() const = 0;
            /** an url which is unique for this track. Use this if you need a key for the track */
            virtual QString url() const = 0;

            /** Returns whether playableUrl() will return a playable Url */
            virtual bool isPlayable() const = 0;
            /** Returns true if the tags of this track are editable */
            virtual bool isEditable() const = 0;

            /** Returns the album this track belongs to */
            virtual AlbumPtr album() const = 0;
            /** Update the album of this track. */
            virtual void setAlbum( const QString &newAlbum ) = 0;
            //TODO: add overloaded methods which take a AlbumPtr if necessary
            /** Returns the artist of this track */
            virtual ArtistPtr artist() const = 0;
            /** Change the artist of this track */
            virtual void setArtist( const QString &newArtist ) = 0;
            /** Returns the composer of this track */
            virtual ComposerPtr composer() const = 0;
            virtual void setComposer( const QString &newComposer ) = 0;
            /** Returns the genre of this track */
            virtual GenrePtr genre() const = 0;
            virtual void setGenre( const QString &newGenre ) = 0;

            /** Returns the score of this track */
            virtual double score() const = 0;
            virtual void setScore( double newScore ) = 0;
            /** Returns the ratint of this track */
            virtual int rating() const = 0;
            virtual void setRating( int newRating ) = 0;
            /** Returns the length of this track in seconds, or 0 if unknown */
            virtual int length() const = 0;
            /** Returns the filesize of this track in bytes */
            virtual int filesize() const = 0;
            /** Returns the sample rate of this track */
            virtual int sampleRate() const = 0;
            /** Returns the bitrate o this track */
            virtual int bitrate() const = 0;
            /** Returns the track number of this track */
            virtual int trackNumber() const = 0;
            virtual void setTrackNumber( int newTrackNumber ) = 0;
            /** Returns the discnumber of this track */
            virtual int discNumber() const = 0;
            virtual void setDiscNumber( int newDiscNumber ) = 0;
            virtual uint lastPlayed() const = 0;
            virtual int playCount() const = 0;

            /** Returns the type of this track, e.g. "ogg", "mp3", "Stream" */
            virtual QString type() const = 0;
    };

    class Artist : public MetaBase
    {
        public:
            typedef QList<ArtistPtr> ArtistList;

            virtual TrackList tracks() = 0;

            virtual void invalidateCache() = 0;
    };

    class Album : public MetaBase
    {
        public:
            typedef QList<AlbumPtr> AlbumList;

            virtual bool isCompilation() const = 0;

            virtual bool hasAlbumArtist() const = 0;
            virtual ArtistPtr albumArtist() const = 0;
            virtual TrackList tracks() = 0;

            virtual void image() const = 0; //TODO: choose return value
            virtual bool canUpdateImage() const { return false; }
            virtual bool updateImage() {} //TODO: choose parameter

            virtual void invalidateCache() = 0;
    };

    class Composer : public MetaBase
    {
        public:
            typedef QList<ComposerPtr> ComposerList;
            virtual TrackList tracks() const = 0;

            virtual void invalidateCache() = 0;
    };

    class Genre : public MetaBase
    {
        public:
            typedef QList<GenrePtr> GenreList;

            virtual TrackList tracks() const = 0;

            virtual void invalidateCache() = 0;
    };
}

#endif /* META_H */