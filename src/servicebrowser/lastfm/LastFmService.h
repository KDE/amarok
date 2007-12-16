/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMSERVICE_H
#define LASTFMSERVICE_H

#include "../servicebase.h"

#include <kio/jobclasses.h>
#include <kio/job.h>

class LastFmServiceFactory : public ServiceFactory
{
    Q_OBJECT

public:
    LastFmServiceFactory() {}
    virtual ~LastFmServiceFactory() {}

    virtual void init();
    virtual QString name();
    virtual KPluginInfo info();
    virtual KConfigGroup config();
};

class LastFmService : public ServiceBase
{
    Q_OBJECT

public:
    LastFmService( const QString &name );
    virtual ~LastFmService();

    virtual void polish();
};

#endif // LASTFMSERVICE_H