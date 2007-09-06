//
// C++ Implementation: DynamicServiceCollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "ShoutcastServiceCollection.h"

#include "ShoutcastServiceQueryMaker.h"

ShoutcastServiceCollection::ShoutcastServiceCollection()
 : ServiceCollection()
{
}


ShoutcastServiceCollection::~ShoutcastServiceCollection()
{
}

QueryMaker * ShoutcastServiceCollection::queryMaker()
{
    return new ShoutcastServiceQueryMaker( this );
}

QString ShoutcastServiceCollection::collectionId() const
{
    return "Shoutcast collection";
}

QString ShoutcastServiceCollection::prettyName() const
{
    return collectionId();
}


