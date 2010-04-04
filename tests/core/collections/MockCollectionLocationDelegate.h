




#ifndef MOCKCOLLECTIONLOCATIONDELEGATE_H
#define MOCKCOLLECTIONLOCATIONDELEGATE_H

#include "core/collections/CollectionLocationDelegate.h"

#include <gmock/gmock.h>

namespace Collections {

class MockCollectionLocationDelegate : public CollectionLocationDelegate
{
public:
    MOCK_CONST_METHOD2( reallyDelete, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( reallyMove, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( errorDeleting, void( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD1( notWriteable, void( CollectionLocation *loc ) );
    MOCK_CONST_METHOD1( deleteEmptyDirs, bool( CollectionLocation *loc ) );
};

} //namespace Collections

#endif
