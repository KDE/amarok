/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef METAMULTISOURCECAPABILITY_H
#define METAMULTISOURCECAPABILITY_H

#include <Capability.h>

#include <KUrl>

namespace Meta {

/**
A capability for tracks that can have several different source urls, such as multiple fallback streams for a radio station. If one source url fails or finishes, the track will automatically use the next one. It is also possbile to get a list of all urls that can be presented to the user so he can choose.

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MultiSourceCapability : public Capability
{
    Q_OBJECT
public:
    MultiSourceCapability();

    ~MultiSourceCapability();

    static Type capabilityInterfaceType() { return Meta::Capability::MultiSource; }

    virtual KUrl first() = 0;
    virtual KUrl next() = 0;
    virtual int current() = 0;
    virtual QStringList tracks() = 0;

};

}

#endif
