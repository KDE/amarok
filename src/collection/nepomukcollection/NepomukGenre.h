/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#ifndef NEPOMUKGENRE_H_
#define NEPOMUKGENRE_H_

#include "Meta.h"

#include "QString"

namespace Meta
{

class NepomukGenre : public Genre
{
    public:
        NepomukGenre( const QString &name );
        
        virtual QString name() const;
        virtual QString prettyName() const;
        
        virtual TrackList tracks();
        
    private:
        QString m_name;
};

}
#endif /*NEPOMUKENRE_H_*/
