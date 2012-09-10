/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#include <QSharedPointer>

#include <Nepomuk/Resource>

namespace Meta
{

class NepomukTrack;
typedef KSharedPtr<NepomukTrack> NepomukTrackPtr;
typedef QList<NepomukTrackPtr> NepomukTrackList;

/**
 * Represents a unit music track resource in Amarok
 */

class NepomukTrack : public Track
{
public:
    // construct a NepomukTrack out of a Nepomuk resource
    NepomukTrack( const QUrl &resUri, NepomukCollection *coll );
    // construct a NepomukTrack out of a url

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

    virtual qreal bpm() const;
    virtual QString comment() const;
    virtual double score() const;
    virtual void setScore( double newScore );
    virtual int rating() const;
    virtual void setRating( int newRating );
    virtual qint64 length() const;
    virtual int filesize() const;
    virtual int sampleRate() const;
    virtual int bitrate() const;
    virtual QDateTime createDate() const;
    virtual QDateTime modifyDate() const;
    virtual int trackNumber() const;
    virtual int discNumber() const;
    virtual int playCount() const;
    virtual QString type() const;

    // NepomukTrack methods

    void setAlbum( NepomukAlbumPtr album );
    void setArtist( NepomukArtistPtr artist );
    void setComposer( NepomukComposerPtr composer );
    void setGenre( NepomukGenrePtr genre );
    void setYear( NepomukYearPtr year );

    void setName( const QString &name );
    void setType( const QString &type );
    void setLength( const qint64 length );
    void setBitrate( const int rate );
    void setTrackNumber( const int trackNumber );
    void setUidUrl( const QString &uidUrl );
    void setDiscNumber( const int discNumber );
    void setModifyDate( const QDateTime &modifyDate );
    void setCreateDate( const QDateTime &createDate );
    void setbpm( const qreal bpm );
    void setComment( const QString &comment );
    void setSampleRate( const int sampleRate );
    void setFilesize( const int filesize );
    void setTrackGain( const double trackGain );
    void setTrackPeakGain( const double trackPeakGain );
    void setAlbumGain( const double albumGain );
    void setAlbumPeakGain( const double albumPeakGain );
    void setPlayableUrl( const KUrl &url );

    // Non pure virtual functions
    virtual bool inCollection() const;
    /**
     * This should be implemented whenever you return true in inCollection()
     */
    virtual Collections::Collection *collection() const;
    virtual qreal replayGain( ReplayGainTag mode ) const;

    virtual void addLabel( const Meta::LabelPtr &label );
    virtual void addLabel( const QString &label );
    virtual Meta::LabelList labels() const;
    virtual void removeLabel( const Meta::LabelPtr &label );

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
