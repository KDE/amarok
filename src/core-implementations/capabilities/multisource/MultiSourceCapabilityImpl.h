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

#include "core/support/Debug.h"
#include "core-implementations/meta/multi/MultiTrack.h"

namespace Capabilities {

class MultiSourceCapabilityImpl : public Capabilities::MultiSourceCapability
{
    Q_OBJECT
public:
    MultiSourceCapabilityImpl( Meta::MultiTrack * track );


    virtual KUrl first() { return m_track->first(); }
    virtual KUrl next() { return m_track->next(); }
    virtual int current() { return m_track->current(); }
    virtual QStringList sources() { return m_track->sources(); }
    virtual void setSource( int source );

private:
    Meta::MultiTrack * m_track;

};

}

#endif


