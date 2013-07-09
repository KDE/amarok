#ifndef AUDIOCDALBUM_H
#define AUDIOCDALBUM_H

#include "core/meta/Meta.h"
#include "AudioCdCollection.h"
#include "AudioCdTrack.h"

using namespace Meta;

class AudioCdAlbum : public Meta::Album
{
    public:
        AudioCdAlbum( const QString &name, const QString &albumArtist );
        virtual ~AudioCdAlbum();

        virtual QString name() const;

        virtual bool isCompilation() const;
        virtual bool canUpdateCompilation() const;
        virtual void setCompilation( bool compilation );

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QImage image( int size = 0 ) const;
        virtual bool hasImage( int size = 0 ) const;
        virtual bool canUpdateImage() const;

        //AudioCdAlbum specific methods
        void setAlbumArtist( QString& artist );

    private:
        QString m_name;
        QString m_albumArtist;
        bool m_isCompilation;
};
#endif
