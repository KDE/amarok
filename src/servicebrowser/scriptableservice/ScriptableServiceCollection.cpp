//
// C++ Implementation: scriptableservicecollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "ScriptableServiceCollection.h"
#include "support/memoryquerymaker.h"


ScriptableServiceCollection::ScriptableServiceCollection(const QString & name)
    : Collection()
    , MemoryCollection()
{ 
    m_name = name;
}


ScriptableServiceCollection::~ScriptableServiceCollection()
{
}

QueryMaker*
ScriptableServiceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString ScriptableServiceCollection::prettyName() const
{
    return m_name;
}

QString ScriptableServiceCollection::collectionId() const
{
    return m_name;
}


bool ScriptableServiceCollection::possiblyContainsTrack(const KUrl & url) const
{
    return false;
}

Meta::TrackPtr ScriptableServiceCollection::ScriptableServiceCollection::trackForUrl(const KUrl & url)
{
    return TrackPtr();
}

CollectionLocation * ScriptableServiceCollection::ScriptableServiceCollection::location() const
{
    return 0;
}

void ScriptableServiceCollection::emitUpdated()
{
    emit( updated() );

}
