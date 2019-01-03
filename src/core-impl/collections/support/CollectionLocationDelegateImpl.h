/****************************************************************************************
 * Copyright (c) 2012 Ryan McCoskrie <ryan.mccoskrie@gmail.com>                         *
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef COLLECTIONLOCATIONDELEGATEIMPL_H
#define COLLECTIONLOCATIONDELEGATEIMPL_H

#include "amarok_export.h"
#include "core/collections/CollectionLocationDelegate.h"

#include <QStringList>

namespace Collections {

class AMAROK_EXPORT CollectionLocationDelegateImpl : public CollectionLocationDelegate
{
public:
    CollectionLocationDelegateImpl() {};
    virtual ~ CollectionLocationDelegateImpl() {};

    bool reallyDelete( CollectionLocation *loc, const Meta::TrackList &tracks ) const override;
    bool reallyMove(CollectionLocation* loc, const Meta::TrackList& tracks) const override;
    bool reallyTrash( CollectionLocation *loc, const Meta::TrackList &tracks ) const override;
    void errorDeleting( CollectionLocation* loc, const Meta::TrackList& tracks ) const override;
    void notWriteable(CollectionLocation* loc) const override;
    bool deleteEmptyDirs(CollectionLocation* loc) const override;
    Transcoding::Configuration transcode( const QStringList &playableFileTypes,
                                                  bool *remember, OperationType operation,
                                                  const QString &destCollectionName,
                                                  const Transcoding::Configuration &prevConfiguration ) const override;

private:
    QStringList trackList( const Meta::TrackList &tracks ) const;
};

} //namespace Collections

#endif
