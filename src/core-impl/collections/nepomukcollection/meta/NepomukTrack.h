/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukCollection.h"
#include "NepomukGenre.h"
#include "NepomukComposer.h"
#include "NepomukAlbum.h"
#include "NepomukArtist.h"
#include "NepomukYear.h"

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"

#include <QSharedPointer>

#include <Nepomuk/Resource>

namespace Meta
{
class NepomukTrack;
typedef KSharedPtr<NepomukTrack> NepomukTrackPtr;

/**
 * Represents a unit music track resource in Amarok
 */
class NepomukTrack : public Track, public Statistics
{
public:
    // construct a NepomukTrack out of a Nepomuk resource
    NepomukTrack( const QUrl &resUri, NepomukCollection *coll );
    ~NepomukTrack();

    virtual QString name() const;
    virtual KUrl playableUrl() const;
    virtual QString prettyUrl() const;
    virtual QString uidUrl() const;

    virtual bool isPlayable() const;
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
    // TODO: switch to default finishedPlaying() implementation from Meta::Track once
    // we implement setPlaycount(), setLastPlayed().
    virtual void finishedPlaying( double playedFraction );

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

    // TODO: implement (set)First/LastPlayed()

    virtual int playCount() const;
    // TODO: implement setPlayCount();

    // TODO: implement beginUpdate()/endUpdate() once other stats methods are here

    // NepomukTrack meta methods
    void setAlbum( AlbumPtr album );
    void setArtist( ArtistPtr artist );
    void setComposer( ComposerPtr composer );
    void setGenre( GenrePtr genre );
    void setYear( YearPtr year );

    // NepomukTrack secondary metadata methods
    void setName( const QString &name );
    void setType( const QString &type );
    void setLength( const qint64 length );
    void setBitrate( int rate );
    void setTrackNumber( int trackNumber );
    void setUidUrl( const QString &uidUrl );
    void setDiscNumber( int discNumber );
    void setModifyDate( const QDateTime &modifyDate );
    void setCreateDate( const QDateTime &createDate );
    void setbpm( const qreal bpm );
    void setComment( const QString &comment );
    void setSampleRate( int sampleRate );
    void setFilesize( int filesize );
    void setTrackGain( qreal trackGain );
    void setTrackPeakGain( qreal trackPeakGain );
    void setAlbumGain( qreal albumGain );
    void setAlbumPeakGain( qreal albumPeakGain );
    void setPlayableUrl( const KUrl &url );

private:

    ArtistPtr m_artist;
    GenrePtr m_genre;
    ComposerPtr m_composer;
    AlbumPtr m_album;
    YearPtr m_year;
    LabelList m_labellist;

    KUrl m_playableUrl;
    QString m_name;
    QString m_type;
    qint64 m_length;
    int m_bitrate;
    int m_trackNumber;
    QString m_uidUrl;
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

    NepomukCollection *m_coll;
    Nepomuk::Resource m_resource;
};

}
#endif /*NEPOMUKTRACK_H*/
