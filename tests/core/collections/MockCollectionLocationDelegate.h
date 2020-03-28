/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>                             *
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





#ifndef MOCKCOLLECTIONLOCATIONDELEGATE_H
#define MOCKCOLLECTIONLOCATIONDELEGATE_H

#include "core/collections/CollectionLocationDelegate.h"

#undef kWarning  // WORKAROUND: Prevent symbols clash with KDE's kWarning macro
#include <gmock.h>

namespace Collections {

class MockCollectionLocationDelegate : public CollectionLocationDelegate
{
public:
    MOCK_CONST_METHOD2( reallyDelete, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( reallyMove, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( reallyTrash, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( errorDeleting, void( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD1( notWriteable, void( CollectionLocation *loc ) );
    MOCK_CONST_METHOD1( deleteEmptyDirs, bool( CollectionLocation *loc ) );
    MOCK_CONST_METHOD5( transcode, Transcoding::Configuration(
        const QStringList &playableFileTypes, bool *remember, OperationType operation,
        const QString &destCollectionName, const Transcoding::Configuration &prevConfiguration ) );
};

} //namespace Collections

#endif
