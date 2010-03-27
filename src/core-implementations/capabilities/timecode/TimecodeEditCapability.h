/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef TIMECODEEDITCAPABILITY_H
#define TIMECODEEDITCAPABILITY_H

#include "core-implementations/meta/timecode/TimecodeMeta.h"
#include "core/capabilities/EditCapability.h"

namespace Capabilities {

class AMAROK_EXPORT TimecodeEditCapability : public EditCapability
{
Q_OBJECT
public:
    TimecodeEditCapability( Meta::TimecodeTrack * track );
    ~TimecodeEditCapability() {}

    static Type capabilityInterfaceType() { return Capabilities::Capability::Editable; }

    virtual bool isEditable() const { return true; }
    virtual void setAlbum( const QString &newAlbum );
    virtual void setArtist( const QString &newArtist );
    virtual void setComposer( const QString &newComposer );
    virtual void setGenre( const QString &newGenre );
    virtual void setYear( const QString &newYear );
    virtual void setBpm( const qreal Bpm );
    virtual void setTitle( const QString &newTitle );
    virtual void setComment( const QString &newComment );
    virtual void setTrackNumber( int newTrackNumber );
    virtual void setDiscNumber( int newDiscNumber );

    virtual void beginMetaDataUpdate();
    virtual void endMetaDataUpdate();
    virtual void abortMetaDataUpdate();

private:
    Meta::TimecodeTrack * m_track;
};

}
#endif
