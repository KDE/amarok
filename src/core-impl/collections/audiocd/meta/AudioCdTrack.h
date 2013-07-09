#ifndef AUDIOCDTRACK_H
#define AUDIOCDTRACK_H

#include "core/meta/Meta.h"
#include "AudioCdMetaClass.h"

using namespace Meta;

class AudioCdCollection;
class AudioCdTrack;
typedef KSharedPtr<AudioCdTrack> AudioCdTrackPtr;

class AudioCdAlbum;
typedef KSharedPtr<AudioCdAlbum> AudioCdAlbumPtr;

class AudioCdTrack : public Meta::Track
{
    public:
        AudioCdTrack( AudioCdCollection *collection, const KUrl &url );
        virtual ~AudioCdTrack();

        virtual QString name() const;

        virtual KUrl playableUrl() const;
        virtual QString uidUrl() const;
        virtual QString prettyUrl() const;
        virtual QString notPlayableReason() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        virtual qreal bpm() const;

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual qint64 length() const;

        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual QString type() const;

        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

        //AudioCdTrack specific methods
        void setAlbum( AudioCdAlbumPtr album );
        void setArtist( const QString& artist );
        void setComposer( const QString& composer );
        void setGenre( const QString& genre );
        void setTitle( const QString& title );
        void setYear( const QString& year );

        void setLength( qint64 length );

    private:
        QWeakPointer<AudioCdCollection> m_collection;
        AudioCdAlbumPtr m_album;

        QString m_artist;
        QString m_genre;
        QString m_composer;
        QString m_year;
        QString m_name;
        qint64 m_length;
        int m_trackNumber;
        KUrl m_playableUrl;
};
#endif
