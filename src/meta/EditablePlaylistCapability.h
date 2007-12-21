/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef METAEDITABLEPLAYLISTCAPABILITY_H
#define METAEDITABLEPLAYLISTCAPABILITY_H

#include <Capability.h>
#include <Meta.h>

#include <QDateTime>

class QString;
class KUrl;

namespace Meta {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class EditablePlaylistCapability : public Capability
{
public:

    virtual ~EditablePlaylistCapability();

    static Type capabilityInterfaceType() { return Meta::Capability::EditablePlaylist; }

    /** Returns true if the tags of this track are currently editable */
    virtual bool isEditable() const = 0;

    virtual void setTitle( QString title ) {};
    virtual void setCreator( QString creator ) {};
    virtual void setAnnotation( QString annotation ) {};
    virtual void setInfo( KUrl info ) {};
    virtual void setLocation( KUrl location ) {};
    virtual void setIdentifier( QString identifier ) {};
    virtual void setImage( KUrl image ) {};
    virtual void setDate( QDateTime date ) {};
    virtual void setLicense( KUrl license ) {};
    virtual void setAttribution( KUrl attribution, bool append = true ) {};
    virtual void setLink( KUrl link ) {};
    virtual void setTrackList( TrackList trackList, bool append = false ) {};

    /** The playlist object should not store changed meta data immediately but cache the
    changes until endMetaDataUpdate() or abortMetaDataUpdate() is called */
    virtual void beginMetaDataUpdate() = 0;
    /** All meta data has been updated and the object should commit the changed */
    virtual void endMetaDataUpdate() = 0;
    /** Abort the meta data update without committing the changes */
    virtual void abortMetaDataUpdate() = 0;

};

}

#endif
