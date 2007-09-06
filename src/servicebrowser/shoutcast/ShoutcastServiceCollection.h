//
// C++ Interface: DynamicServiceCollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DYNAMICSERVICECOLLECTION_H
#define DYNAMICSERVICECOLLECTION_H

#include <servicecollection.h>

/**
A collection that dynamically fetches data from a remote location as needed

	@author 
*/
class ShoutcastServiceCollection : public ServiceCollection
{
public:
    ShoutcastServiceCollection();

    ~ShoutcastServiceCollection();

    virtual QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;

};

#endif
