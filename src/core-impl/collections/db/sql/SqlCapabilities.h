/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef SQLCAPABILITIES_H
#define SQLCAPABILITIES_H

#include "amarok_sqlcollection_export.h"
#include "amarokurls/PlayUrlRunner.h"
#include "core/capabilities/Capability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/capabilities/OrganiseCapability.h"
#include "core/meta/TrackEditor.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "core-impl/collections/db/sql/SqlMeta.h"

/* note: currently SqlTrack has the following capabilities
    EditCapabilityImpl
    ActionsCapability
    OrganiseCapabilityImpl
    BookmarkThisCapability
    TimecodeWriteCapabilityImpl
    TimecodeLoadCapabilityImpl
    SqlReadLabelCapability
    SqlWriteLabelCapability
    FindInSourceCapabilityImpl
*/

namespace Capabilities {

class EditCapabilityImpl : public Meta::TrackEditor
{
    Q_OBJECT

    public:

    EditCapabilityImpl( Meta::SqlTrack *track );
    virtual ~EditCapabilityImpl();

    virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
    virtual void setAlbumArtist( const QString &newAlbumArtist ) { m_track->setAlbumArtist( newAlbumArtist ); }
    virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
    virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
    virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
    virtual void setYear( int newYear ) { m_track->setYear( newYear ); }
    virtual void setBpm( const qreal newBpm ) { m_track->setBpm( newBpm ); }
    virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
    virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
    virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
    virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
    virtual void beginUpdate() { m_track->beginUpdate(); }
    virtual void endUpdate() { m_track->endUpdate(); }

    private:
    KSharedPtr<Meta::SqlTrack> m_track;
};

class OrganiseCapabilityImpl : public Capabilities::OrganiseCapability
{
    Q_OBJECT

    public:

    OrganiseCapabilityImpl( Meta::SqlTrack *track );
    virtual ~OrganiseCapabilityImpl();

    virtual void deleteTrack();

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeWriteCapabilityImpl : public Capabilities::TimecodeWriteCapability
{
    Q_OBJECT

    public:

    TimecodeWriteCapabilityImpl( Meta::SqlTrack *track );
    virtual ~TimecodeWriteCapabilityImpl();

    virtual bool writeTimecode( qint64 miliseconds )
    {
        return Capabilities::TimecodeWriteCapability::writeTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    virtual bool writeAutoTimecode( qint64 miliseconds )
    {
        return Capabilities::TimecodeWriteCapability::writeAutoTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeLoadCapabilityImpl : public Capabilities::TimecodeLoadCapability
{
    Q_OBJECT

    public:

    TimecodeLoadCapabilityImpl( Meta::SqlTrack *track );
    virtual ~TimecodeLoadCapabilityImpl();

    virtual bool hasTimecodes();
    virtual QList<KSharedPtr<AmarokUrl> > loadTimecodes();

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};


class FindInSourceCapabilityImpl : public Capabilities::FindInSourceCapability
{
    Q_OBJECT

    public:
    FindInSourceCapabilityImpl( Meta::SqlTrack *track );
    virtual ~FindInSourceCapabilityImpl();

    virtual void findInSource( QFlags<TargetTag> tag );

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};


} //namespace Capabilities

#endif // SQLCAPABILITIES_H
