/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef METAEDITABLEPLAYLISTCAPABILITY_H
#define METAEDITABLEPLAYLISTCAPABILITY_H

#include "amarok_export.h"
#include "meta/Capability.h"
#include "meta/Meta.h"

#include <QDateTime>

class QString;
class KUrl;

namespace Meta {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT EditablePlaylistCapability : public Capability
{
    Q_OBJECT

    public:

        virtual ~EditablePlaylistCapability();

        static Type capabilityInterfaceType() { return Meta::Capability::EditablePlaylist; }

        /** Returns true if the tags of this track are currently editable */
        virtual bool isEditable() const = 0;

        virtual void setTitle( const QString &title ) { Q_UNUSED( title ); };
        virtual void setCreator( const QString &creator ) { Q_UNUSED( creator ); };
        virtual void setAnnotation( const QString &annotation ) { Q_UNUSED( annotation ); };
        virtual void setInfo( const KUrl &info ) { Q_UNUSED( info ); };
        virtual void setLocation( const KUrl &location ) { Q_UNUSED( location); };
        virtual void setIdentifier( const QString &identifier ) { Q_UNUSED( identifier); };
        virtual void setImage( const KUrl &image ) { Q_UNUSED( image ); };
        virtual void setDate( const QDateTime &date ) { Q_UNUSED( date); };
        virtual void setLicense( const KUrl &license ) { Q_UNUSED( license ); };
        virtual void setAttribution( const KUrl &attribution, bool append = true ) { Q_UNUSED( attribution); Q_UNUSED(append); };
        virtual void setLink( const KUrl &link ) { Q_UNUSED( link ); };
        virtual void setTrackList( TrackList trackList, bool append = false ) { Q_UNUSED( trackList ); Q_UNUSED( append ); };

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
