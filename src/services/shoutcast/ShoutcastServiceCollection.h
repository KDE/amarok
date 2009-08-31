/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef SHOUTCASTSERVICECOLLECTION_H
#define SHOUTCASTSERVICECOLLECTION_H

#include <ServiceCollection.h>

/**
A collection that dynamically fetches data from a remote location as needed
*/
class ShoutcastServiceCollection : public ServiceCollection
{
public:
    ShoutcastServiceCollection();
    ShoutcastServiceCollection(bool isTop500);

    ~ShoutcastServiceCollection();

    virtual QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;

private:
    bool m_top500;
};

#endif
