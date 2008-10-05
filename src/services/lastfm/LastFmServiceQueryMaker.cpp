/***************************************************************************
 * copyright            : (C) 2008 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LastFmServiceQueryMaker.h"

LastFmServiceQueryMaker::LastFmServiceQueryMaker( LastFmServiceCollection *collection )
    : m_resultAsDataPtrs( false )
{
    Q_UNUSED( collection );
}


LastFmServiceQueryMaker::~LastFmServiceQueryMaker()
{
}


QueryMaker *
LastFmServiceQueryMaker::reset()
{
    m_resultAsDataPtrs = false;
    return this;
}

    
void 
LastFmServiceQueryMaker::run()
{
}
    

void 
LastFmServiceQueryMaker::abortQuery()
{
}


QueryMaker*
LastFmServiceQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    m_resultAsDataPtrs = resultAsDataPtrs;
    return this;
}
