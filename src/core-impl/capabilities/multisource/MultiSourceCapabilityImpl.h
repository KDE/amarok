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


#ifndef AMAROK_MULTISOURCECAPABILITYIMPL_P_H
#define AMAROK_MULTISOURCECAPABILITYIMPL_P_H

#include "core/capabilities/MultiSourceCapability.h"
#include "core-impl/meta/multi/MultiTrack.h"

namespace Capabilities
{
    class MultiSourceCapabilityImpl : public MultiSourceCapability
    {
        Q_OBJECT

        public:
            explicit MultiSourceCapabilityImpl( Meta::MultiTrack *track );
            ~MultiSourceCapabilityImpl() override;

            QStringList sources() const override;
            void setSource( int source ) override;
            int current() const override;
            QUrl nextUrl() const override;

        private:
            Meta::MultiTrack *m_track;
    };
}

#endif
