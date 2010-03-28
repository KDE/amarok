/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef DYNAMICSCRIPTABLESERVICECOLLECTION_H
#define DYNAMICSCRIPTABLESERVICECOLLECTION_H

#include "../ServiceCollection.h"
#include "AmarokProcess.h"

namespace Collections {

/**
A collection that can call back a script to populate items as needed.

	@author 
*/
class ScriptableServiceCollection : public ServiceCollection
{
    Q_OBJECT
public:
    ScriptableServiceCollection( const QString &name );

    ~ScriptableServiceCollection();

    virtual Collections::QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    void donePopulating( int parentId );

    void setLevels( int theValue ) {
        m_levels = theValue;
    }
    
    int levels() const {
        return m_levels;
    }

    void setLastFilter( const QString & filter ) { m_lastFilter = filter; }
    QString lastFilter() { return m_lastFilter; }

    void clear();

    signals:
        void updateComplete();

private:

    QString m_name;
    int m_levels;
    QString m_lastFilter;

};

} //namespace Collections

#endif
