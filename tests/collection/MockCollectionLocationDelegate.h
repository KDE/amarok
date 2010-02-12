




#ifndef MOCKCOLLECTIONLOCATIONDELEGATE_H
#define MOCKCOLLECTIONLOCATIONDELEGATE_H

#include "collection/CollectionLocationDelegate.h"

#include <gmock/gmock.h>

class MockCollectionLocationDelegate : public CollectionLocationDelegate
{
public:
    MOCK_CONST_METHOD2( reallyDelete, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( reallyMove, bool( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD2( errorDeleting, void( CollectionLocation *loc, const Meta::TrackList &tracks ) );
    MOCK_CONST_METHOD1( notWriteable, void( CollectionLocation *loc ) );
};

#endif
