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
 
#ifndef METAMULTISOURCECAPABILITY_H
#define METAMULTISOURCECAPABILITY_H

#include "core/capabilities/Capability.h"

#include <KUrl>

namespace Capabilities {

/**
A capability for tracks that can have several different source urls, such as multiple fallback streams for a radio station. If one source url fails or finishes, the track will automatically use the next one. It is also possbile to get a list of all urls that can be presented to the user so he can choose.

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MultiSourceCapability : public Capability
{
    Q_OBJECT
public:
    MultiSourceCapability();

    ~MultiSourceCapability();

    static Type capabilityInterfaceType() { return Capabilities::Capability::MultiSource; }

    virtual KUrl first() = 0;
    virtual KUrl next() = 0;
    virtual int current() = 0;
    virtual QStringList sources() = 0;
    virtual void setSource( int source ) = 0;
    
signals:
    void urlChanged( const KUrl &url );

};

}

#endif
