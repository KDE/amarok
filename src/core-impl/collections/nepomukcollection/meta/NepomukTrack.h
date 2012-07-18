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

#include <QDateTime>
#include <QSharedPointer>
#include <QString>

#include <Nepomuk/Resource>
#include <KUrl>

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
    NepomukTrack( Nepomuk::Resource resource );
    // construct a NepomukTrack out of a url

    ~NepomukTrack();

    virtual QString name() const;
    virtual QString prettyName() const;
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
    // TODO
    //void setYear( NepomukYearPtr year );

private:

    Nepomuk::Resource m_resource;
    ArtistPtr m_artist;
    GenrePtr m_genre;
    ComposerPtr m_composer;
    AlbumPtr m_album;
    YearPtr m_year;
    LabelList m_labellist;
    KUrl m_kurl;
    QString m_name;
};

}
#endif /*NEPOMUKTRACK_H*/
