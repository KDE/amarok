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
#ifndef MP3TUNESSERVICECOLLECTION_H
#define MP3TUNESSERVICECOLLECTION_H

#include <ServiceCollectionLocation.h>
#include "Mp3tunesLocker.h"

class Mp3tunesServiceCollection : public ServiceCollection
{
public:
    Mp3tunesServiceCollection( const QString &sessionId, Mp3tunesLocker * locker );


    ~Mp3tunesServiceCollection();

    virtual QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;
    virtual CollectionLocation* location() const;
    Mp3tunesLocker* locker() const;

private:

    QString m_sessionId;
    Mp3tunesLocker * m_locker;

};

#endif
