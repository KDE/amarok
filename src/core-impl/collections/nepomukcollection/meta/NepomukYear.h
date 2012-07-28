/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#ifndef NEPOMUKYEAR_H
#define NEPOMUKYEAR_H

#include "core/meta/Meta.h"
#include "NepomukCollection.h"

#include <Nepomuk/Resource>

using namespace Collections;

namespace Meta
{

class NepomukYear;
typedef KSharedPtr<NepomukYear> NepomukYearPtr;
typedef QList<NepomukYearPtr> NepomukYearList;


class NepomukYear : public Year
{
public:
    NepomukYear( const Nepomuk::Resource resource );
    virtual TrackList tracks();
    virtual QString name() const;

private:
    Nepomuk::Resource m_resource;

};

}

#endif // NEPOMUKYEAR_H
