/****************************************************************************************
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

#ifndef COLLECTIONLOCATIONDELEGATE_H
#define COLLECTIONLOCATIONDELEGATE_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"
#include "core/transcoding/TranscodingConfiguration.h"

namespace Collections {

class CollectionLocation;

class AMAROKCORE_EXPORT CollectionLocationDelegate
{
public:
    enum OperationType {
        Copy,
        Move
    };

    CollectionLocationDelegate() {}
    virtual ~ CollectionLocationDelegate() {}

    virtual bool reallyDelete( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual bool reallyMove( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual bool reallyTrash( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual void errorDeleting( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual void notWriteable( CollectionLocation *loc ) const = 0;
    virtual bool deleteEmptyDirs( CollectionLocation *loc ) const = 0;

    /**
     * Displays a dialog requesting what transcoding configuration to use.
     *
     * @param playableFileTypes list of filetypes that are playable (empty if everything playable)
     * @param remember is set to true if user checks this transcoding config should be
     * remembered per target collection. If null, such option is disabled in the UI.
     * @param operation whether this is copying or moving
     * @param destCollectionName name of the destination collection
     * @param prevConfiguration the previously saved configuration, for restoring values from
     *
     * @return Transcoding configuration user requested or invalid configuration if user
     * has hit Cancel or closed the dialog.
     */
    virtual Transcoding::Configuration transcode( const QStringList &playableFileTypes,
                                                  bool *remember, OperationType operation,
                                                  const QString &destCollectionName,
                                                  const Transcoding::Configuration &prevConfiguration ) const = 0;
};

} //namespace Collections

#endif
