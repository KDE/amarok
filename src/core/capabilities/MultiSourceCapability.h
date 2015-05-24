/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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
 
#ifndef METAMULTISOURCECAPABILITY_H
#define METAMULTISOURCECAPABILITY_H

#include "core/capabilities/Capability.h"

class QUrl;

namespace Capabilities
{
    /**
     * A capability for tracks that can have several different source urls, such as
     * multiple fallback streams for a radio station. If one source url fails or finishes,
     * the track will automatically use the next one. It is also possbile to get a list
     * of all urls that can be presented to the user so tha she can choose.
     *
     * @author Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class AMAROK_CORE_EXPORT MultiSourceCapability : public Capability
    {
        Q_OBJECT

        public:
            MultiSourceCapability();
            virtual ~MultiSourceCapability();

            static Type capabilityInterfaceType() { return MultiSource; }

            /**
             * Return list of displayable urls in this MultiSource. Only for display
             * purposes, don't attempt to play these urls.
             */
            virtual QStringList sources() const = 0;

            /**
             * Set current source. Does nothing if @param current is out of bounds.
             */
            virtual void setSource( int source ) = 0;

            /**
             * Get index of the current source
             */
            virtual int current() const = 0;

            /**
             * Return the url of the next source without actually advancing to it.
             * Returns empty url if the current source is the last one.
             */
            virtual QUrl nextUrl() const = 0;

        signals:
            void urlChanged( const QUrl &url );
    };
}

#endif
