/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef WEIGHTEDSTRING_H
#define WEIGHTEDSTRING_H

#include <QStringList>

#include "UnicornDllExportMacro.h"

/** 
 * @author <max@last.fm>
 */
class UNICORN_DLLEXPORT WeightedString : public QString
{
    
public:
    WeightedString() { u.weighting = -1; }
    
    explicit WeightedString( QString name, int w = -1 ) : QString( name ) { u.weighting = w; }
    
    static WeightedString weighted( QString name, int w )
    {
        WeightedString t( name );
        t.u.weighting = w;
        return t;
    }
    
    static WeightedString counted( QString name, int c )
    {
        WeightedString t( name );
        t.u.count = c;
        return t;
    }

    int count() const { return u.count; }
    int weighting() const { return u.weighting; }

private:

    union
    {
        int weighting;
        int count;
    } u;

};


#endif // WEIGHTEDSTRING_H
