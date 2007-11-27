/*
 * MediaDeviceMeta.h
 *
 * Jamie Faris, 2007
 */


#ifndef MEDIADEVICEMETA_H
#define MEDIADEVICEMETA_H

// Amarok
#include "meta.h"

class MediaDevice;


/**
 * A simple implementation of Meta::Artist for media devices.
 * This implementation holds it's values in member variables where
 * they can be set by the device plugin.
 *
 * Device plugins can extend this class to fetch the data directly
 * from the device or to allow editing the tags.
 */
class MediaDeviceArtist : public Meta::Artist
{
    public:
        MediaDeviceArtist( QString name )
            : m_name( name ),
              m_trackList()
        {
        }

        //from Meta::MetaBase
        QString name() const { return m_name; }
        QString prettyName() const { return name(); }

        //from Meta::Artist
        Meta::TrackList tracks() { return m_trackList; }
        Meta::AlbumList albums() { return Meta::AlbumList(); }

    private:
        QString m_name;
        Meta::TrackList m_trackList;
};

/**
 * A simple implementation of Meta::Track for media devices.
 * This implementation holds it's values in member variables where
 * they can be set by the device plugin.
 *
 * Device plugins can extend this class to fetch the data directly
 * from the device and/or allow editing of the tags.
 */
class MediaDeviceTrack : public Meta::Track
{
    public:
        MediaDeviceTrack( MediaDevice *device, QString title )
            : m_device( device ),
              m_name( title )
        {
        }

        //from Meta::MetaBase
        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return name(); }

        //from Meta::Track
        virtual KUrl playableUrl() const { return KUrl(); }
        virtual QString prettyUrl() const { return QString(); }
        virtual QString url() const { return QString(); }
        virtual bool isPlayable() const { return false; }
        virtual Meta::AlbumPtr album() const { return Meta::AlbumPtr(); }
        virtual Meta::ArtistPtr artist() const { return Meta::ArtistPtr(); }
        virtual Meta::ComposerPtr composer() const { return Meta::ComposerPtr(); }
        virtual Meta::GenrePtr genre() const { return Meta::GenrePtr(); }
        virtual Meta::YearPtr year() const { return Meta::YearPtr(); }
        virtual QString comment() const { return QString(); }
        virtual double score() const { return 0; }
        virtual void setScore( double newScore ) { Q_UNUSED(newScore); return; }
        virtual int rating() const { return 0; }
        virtual void setRating( int newRating ) { Q_UNUSED(newRating); return; }
        virtual int length() const { return 0; }
        virtual int filesize() const { return 0; }
        virtual int sampleRate() const { return 0; }
        virtual int bitrate() const { return 0; }
        virtual int trackNumber() const { return 0; }
        virtual int discNumber() const { return 0; }
        virtual uint lastPlayed() const { return 0; }
        virtual uint firstPlayed() const { return 0; }
        virtual int playCount() const { return 0; }
        virtual QString type() const { return QString(); }

    private:
        MediaDevice *m_device;
        QString m_name;
};


#endif //MEDIADEVICEMETA_H
