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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef WEIGHTEDSTRINGLIST_H
#define WEIGHTEDSTRINGLIST_H

#include "WeightedString.h"

#include <QStringList>

// The MSVC compiler will moan about some functions in qlist.h that are
// missing definitions unless we disable this warning. Believe it's a bug in
// Qt. See http://lists.trolltech.com/qt-interest/2006-10/thread00112-0.html.
#ifdef WIN32
    #pragma warning(disable : 4661)
#endif

/** 
 * @author <max@last.fm>
 */
class UNICORN_DLLEXPORT WeightedStringList : public QList<WeightedString>
{
public:
    WeightedStringList() {}
    WeightedStringList( QList<WeightedString> list ) : QList<WeightedString>( list ) {}

    operator QStringList() 
    {
        QStringList strings;
        foreach (WeightedString t, *this)
            strings << QString(t);
        return strings;
    }

    void sortWeightingAscending() 
    {
        qSort( begin(), end(), weightLessThan ); 
    }
    
    void sortWeightingDescending() 
    {
        qSort( begin(), end(), weightMoreThan );
    }
    
    void sortAscending()
    {
        qSort( begin(), end(), caseInsensitiveLessThan );
    }
    
    void sortDescending()    
    {
        qSort( begin(), end(), qGreater<WeightedString>() );        
    }
    
    static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
    {
        return s1.toLower() < s2.toLower();
    } 
    
    static bool weightLessThan(const WeightedString &s1, const WeightedString &s2)
    {
        return s1.weighting() < s2.weighting();
    }
    
    static bool weightMoreThan(const WeightedString &s1, const WeightedString &s2)
    {
        return s1.weighting() > s2.weighting();
    }
};

#endif // WEIGHTEDSTRINGLIST_H
