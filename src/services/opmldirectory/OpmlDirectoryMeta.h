/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef OPMLDIRECTORYMETA_H
#define OPMLDIRECTORYMETA_H


#include "../ServiceMetaBase.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>

class OpmlDirectoryService;

class OpmlDirectoryMetaFactory : public ServiceMetaFactory
{
    public:
        OpmlDirectoryMetaFactory( const QString &dbPrefix, OpmlDirectoryService *service );
        virtual ~OpmlDirectoryMetaFactory() {}

        virtual Meta::TrackPtr createTrack( const QStringList &rows );
        virtual Meta::AlbumPtr createAlbum( const QStringList &rows );
};


namespace Meta
{
class OpmlDirectoryFeed;
class OpmlDirectoryCategory;

typedef AmarokSharedPointer<OpmlDirectoryFeed> OpmlDirectoryFeedPtr;
typedef AmarokSharedPointer<OpmlDirectoryCategory> OpmlDirectoryCategoryPtr;
class OpmlDirectoryFeed  : public ServiceTrack
{


public:
    explicit OpmlDirectoryFeed( const QString &name );
    explicit OpmlDirectoryFeed( const QStringList &resultRow );
};

class OpmlDirectoryCategory : public ServiceAlbum
{
    public:
        explicit OpmlDirectoryCategory( const QString &name );
        explicit OpmlDirectoryCategory( const QStringList &resultRow );
};

}


#endif
