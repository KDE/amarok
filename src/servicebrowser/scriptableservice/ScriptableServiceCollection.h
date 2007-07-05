//
// C++ Interface: scriptableservicecollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SCRIPTABLESERVICECOLLECTION_H
#define SCRIPTABLESERVICECOLLECTION_H

#include "collection.h"
#include "support/memorycollection.h"

/**
A collection for use by the scriptable service. Stores everything in memory

	@author 
*/
class ScriptableServiceCollection : public Collection, public MemoryCollection
{
public:
    ScriptableServiceCollection( const QString &name );

    virtual ~ScriptableServiceCollection();

    virtual QueryMaker * queryMaker();
    virtual void startFullScan() { }

    virtual QString collectionId() const;
    virtual QString prettyName() const;
  
    virtual bool possiblyContainsTrack( const KUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const KUrl &url );

    virtual CollectionLocation* location() const;

    void emitUpdated(); 

    

private:

    QString m_name;

};

#endif
