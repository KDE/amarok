/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
#ifndef DYNAMICSCRIPTABLESERVICECOLLECTION_H
#define DYNAMICSCRIPTABLESERVICECOLLECTION_H

#include "../ServiceCollection.h"
#include "AmarokProcess.h"

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

    virtual QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    void donePopulating( int parentId );

    void setLevels( int theValue ) {
        m_levels = theValue;
    }
    
    int levels() const {
        return m_levels;
    }
    

    signals:
        void updateComplete();

private:

    QString m_name;
    int m_levels;

};

#endif
