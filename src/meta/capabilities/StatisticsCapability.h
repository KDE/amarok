/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_STATISTICSCAPABILITY_H
#define AMAROK_STATISTICSCAPABILITY_H

#include "amarok_export.h"
#include "capabilities/Capability.h"

namespace Meta
{
    class AMAROK_EXPORT StatisticsCapability : public Meta::Capability
    {
        Q_OBJECT
        public:
            virtual ~StatisticsCapability();

            static Type capabilityInterfaceType() { return Meta::Capability::Importable; }

            virtual void setScore( const int score ) = 0;
            virtual void setRating( const int rating ) = 0;
            virtual void setFirstPlayed( const uint time ) = 0;
            virtual void setLastPlayed( const uint time ) = 0;
            virtual void setPlayCount( const int playcount ) = 0;

            /** The track object should not store changed meta data immediately but cache the
            changes until endStatisticsUpdate() or abortStatisticsUpdate() is called */
            virtual void beginStatisticsUpdate() = 0;
            /** All meta data has been updated and the object should commit the changed */
            virtual void endStatisticsUpdate() = 0;
            /** Abort the meta data update without committing the changes */
            virtual void abortStatisticsUpdate() = 0;
    };
}

#endif
