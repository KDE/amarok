/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef AMAROK_CURRENTTRACKACTIONCAPABILITY_H
#define AMAROK_CURRENTTRACKACTIONCAPABILITY_H

#include "CustomActionsCapability.h"
#include "amarok_export.h"

/**
 * This capability allows tracks to display custom actions in different places ( such as the playlist context menu,
 * the system tray context menu and others ) when they are the currently playing tracks. Initial use for this will
 * to add last.fm skip, ban and love actions.
 */

namespace Meta {
    
    class AMAROK_EXPORT CurrentTrackActionsCapability : public CustomActionsCapability {

        Q_OBJECT
        public:
            CurrentTrackActionsCapability();
            CurrentTrackActionsCapability( const QList< QAction* > &actions );

            static Type capabilityInterfaceType() { return Meta::Capability::CurrentTrackActions; }
    };

}


#endif
