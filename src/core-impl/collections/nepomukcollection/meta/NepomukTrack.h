/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>                             *
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
#include "core/meta/Statistics.h"

#include <QScopedPointer>

namespace Collections { class NepomukCollection; }

namespace Nepomuk2 { class Resource; }

namespace Meta
{

class NepomukTrack;

/**
 * Represents a unit music track resource in Amarok
 */
class NepomukTrack : public Track, public Statistics
{
public:
    // construct a NepomukTrack out of a Nepomuk resource
    explicit NepomukTrack( const QUrl &resUri, Collections::NepomukCollection *coll = 0 );
    ~NepomukTrack();

    virtual QString name() const;
    virtual QUrl playableUrl() const;
    virtual QString prettyUrl() const;
    virtual QString uidUrl() const;
    virtual QString notPlayableReason() const;

    virtual AlbumPtr album() const;
    virtual ArtistPtr artist() const;
    virtual ComposerPtr composer() const;
    virtual GenrePtr genre() const;
    virtual YearPtr year() const;

    virtual LabelList labels() const;
    virtual qreal bpm() const;
    virtual QString comment() const;
    virtual qint64 length() const;
    virtual int filesize() const;
    virtual int sampleRate() const;
    virtual int bitrate() const;
    virtual QDateTime createDate() const;
    virtual QDateTime modifyDate() const;
    virtual int trackNumber() const;
    virtual int discNumber() const;
    virtual qreal replayGain( ReplayGainTag mode ) const;

    virtual QString type() const;

    virtual bool inCollection() const;
    virtual Collections::Collection *collection() const;

    virtual void addLabel( const LabelPtr &label );
    virtual void addLabel( const QString &label );
    virtual void removeLabel( const LabelPtr &label );

    virtual StatisticsPtr statistics();

    // Meta::Statistics methods
    // TODO: introduce scores into Nepomuk and implement score(), setScore()

    virtual int rating() const;
    virtual void setRating( int newRating );

    virtual QDateTime lastPlayed() const;
    virtual void setLastPlayed( const QDateTime &date );
    virtual QDateTime firstPlayed() const;
    virtual void setFirstPlayed( const QDateTime &date );

    virtual int playCount() const;
    virtual void setPlayCount( int newPlayCount );

    // NepomukTrack meta methods
    void setAlbum( AlbumPtr album ) { m_album = album; }
    void setArtist( ArtistPtr artist ) { m_artist = artist; }
    void setComposer( ComposerPtr composer ) { m_composer = composer; }
    void setGenre( GenrePtr genre ) { m_genre = genre; }
    void setYear( YearPtr year ) { m_year = year; }

    // NepomukTrack secondary metadata methods
    void setName( const QString &name ) { m_name = name; }
    void setType( const QString &type ) { m_type = type; }
    void setLength( const qint64 length ) { m_length = length; }
    void setBitrate( int rate ) { m_bitrate = rate; }
    void setTrackNumber( int trackNumber ) { m_trackNumber = trackNumber; }
    void setDiscNumber( int discNumber ) { m_discNumber = discNumber; }
    void setModifyDate( const QDateTime &modifyDate ) { m_modifyDate = modifyDate; }
    void setCreateDate( const QDateTime &createDate ) { m_createDate = createDate; }
    void setbpm( const qreal bpm ) { m_bpm = bpm; }
    void setComment( const QString &comment ) { m_comment = comment; }
    void setSampleRate( int sampleRate ) { m_sampleRate = sampleRate; }
    void setFilesize( int filesize ) { m_filesize = filesize; }
    void setTrackGain( qreal trackGain ) { m_trackGain = trackGain; }
    void setTrackPeakGain( qreal trackPeakGain ) { m_trackPeakGain = trackPeakGain; }
    void setAlbumGain( qreal albumGain ) { m_albumGain = albumGain; }
    void setAlbumPeakGain( qreal albumPeakGain ) { m_albumPeakGain = albumPeakGain; }
    void setPlayableUrl( const QUrl &url ) { m_playableUrl = url; }

    bool isFilled(){ return m_filled; }

    void fill( const QString &name, const QUrl &url, Collections::NepomukCollection *coll )
    {
        m_name = name;
        m_playableUrl = url;
        m_coll = coll;
        m_filled = true;
    }

    Nepomuk2::Resource *resource() const;

private:
    bool m_filled;

    ArtistPtr m_artist;
    GenrePtr m_genre;
    ComposerPtr m_composer;
    AlbumPtr m_album;
    YearPtr m_year;
    LabelList m_labellist;

    QUrl m_playableUrl;
    QString m_name;
    QString m_type;
    qint64 m_length;
    int m_bitrate;
    int m_trackNumber;
    int m_discNumber;
    QDateTime m_modifyDate;
    QDateTime m_createDate;
    qreal m_bpm;
    QString m_comment;
    int m_sampleRate;
    int m_filesize;
    double m_trackGain;
    double m_trackPeakGain;
    double m_albumGain;
    double m_albumPeakGain;

    Collections::NepomukCollection *m_coll;
    mutable QScopedPointer<Nepomuk2::Resource> m_resource;
    QUrl m_resourceUri;
};

}
#endif /*NEPOMUKTRACK_H*/
