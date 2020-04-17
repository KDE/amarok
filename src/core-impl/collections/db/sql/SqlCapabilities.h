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

class OrganiseCapabilityImpl : public Capabilities::OrganiseCapability
{
    Q_OBJECT

    public:

    explicit OrganiseCapabilityImpl( Meta::SqlTrack *track );
    ~OrganiseCapabilityImpl() override;

    void deleteTrack() override;

    private:
        AmarokSharedPointer<Meta::SqlTrack> m_track;
};

class TimecodeWriteCapabilityImpl : public Capabilities::TimecodeWriteCapability
{
    Q_OBJECT

    public:

    explicit TimecodeWriteCapabilityImpl( Meta::SqlTrack *track );
    ~TimecodeWriteCapabilityImpl() override;

    bool writeTimecode( qint64 miliseconds ) override
    {
        return Capabilities::TimecodeWriteCapability::writeTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    bool writeAutoTimecode( qint64 miliseconds ) override
    {
        return Capabilities::TimecodeWriteCapability::writeAutoTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    private:
        AmarokSharedPointer<Meta::SqlTrack> m_track;
};

class TimecodeLoadCapabilityImpl : public Capabilities::TimecodeLoadCapability
{
    Q_OBJECT

    public:

    explicit TimecodeLoadCapabilityImpl( Meta::SqlTrack *track );
    ~TimecodeLoadCapabilityImpl() override;

    bool hasTimecodes() override;
    QList<AmarokSharedPointer<AmarokUrl> > loadTimecodes() override;

    private:
        AmarokSharedPointer<Meta::SqlTrack> m_track;
};


class FindInSourceCapabilityImpl : public Capabilities::FindInSourceCapability
{
    Q_OBJECT

    public:
    explicit FindInSourceCapabilityImpl( Meta::SqlTrack *track );
    ~FindInSourceCapabilityImpl() override;

    void findInSource( QFlags<TargetTag> tag ) override;

    private:
        AmarokSharedPointer<Meta::SqlTrack> m_track;
};


} //namespace Capabilities

#endif // SQLCAPABILITIES_H
