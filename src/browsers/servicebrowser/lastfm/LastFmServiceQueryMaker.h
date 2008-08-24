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

#ifndef LASTFMSERVICEQUERYMAKER_H
#define LASTFMSERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

class LastFmServiceCollection;

class LastFmServiceQueryMaker : public DynamicServiceQueryMaker
{
    Q_OBJECT

public:
    LastFmServiceQueryMaker( LastFmServiceCollection *collection );
    virtual ~LastFmServiceQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker *setReturnResultAsDataPtrs( bool resultAsDataPtrs );

private:
    bool m_resultAsDataPtrs;
};

#endif // LASTFMSERVICEQUERYMAKER_H
