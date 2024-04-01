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
#include <gmock/gmock.h>

namespace Collections {

class MockCollectionLocationDelegate : public CollectionLocationDelegate
{
public:
    MOCK_METHOD( bool, reallyDelete, ( CollectionLocation *loc, const Meta::TrackList &tracks ), (const, override));
    MOCK_METHOD( bool, reallyMove, ( CollectionLocation *loc, const Meta::TrackList &tracks ), (const, override) );
    MOCK_METHOD( bool, reallyTrash, ( CollectionLocation *loc, const Meta::TrackList &tracks ), (const, override) );
    MOCK_METHOD( void, errorDeleting, ( CollectionLocation *loc, const Meta::TrackList &tracks ), (const, override) );
    MOCK_METHOD( void, notWriteable, ( CollectionLocation *loc ), (const, override) );
    MOCK_METHOD( bool, deleteEmptyDirs, ( CollectionLocation *loc ), (const, override) );
    MOCK_METHOD( Transcoding::Configuration, transcode, (
        const QStringList &playableFileTypes, bool *remember, OperationType operation,
        const QString &destCollectionName, const Transcoding::Configuration &prevConfiguration ), (const, override) );
};

} //namespace Collections

#endif
