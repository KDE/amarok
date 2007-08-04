/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                                                                         *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DataEngineManager.h"

namespace Context
{
/*
DataEngineManager::DataEngineManager()
    : Plasma::DataEngineManager()
{
}

DataEngineManager::~DataEngineManager()
{
    ;//Plasma::~DateEngineManager();
}

Plasma::DataEngine* DataEngineManager::loadDataEngine(const QString& name)
{
    Plasma::DataEngine* engine = 0;
    Plasma::DataEngine::Dict::const_iterator it = d->engines.find(name);
    
    if (it != d->engines.end()) {
        engine = *it;
        engine->ref();
        return engine;
    }
    
    // load the engine, add it to the engines
    QString constraint = QString("[X-EngineName] == '%1'").arg(name);
    KService::List offers = KServiceTypeTrader::self()->query("Amarok/DataEngine",
        constraint);
    
    if (offers.isEmpty()) {
        kDebug() << "offers are empty for " << name << " with constraint " << constraint;
    } else {
        engine = KService::createInstance<Plasma::DataEngine>(offers.first(), 0);
    }
    
    if (!engine) {
        kDebug() << "Couldn't load engine \"" << name << "\"!";
        return d->nullEngine();
    }
    
    engine->ref();
    engine->setObjectName(offers.first()->name());
    engine->setIcon(offers.first()->icon());
    d->engines[name] = engine;
    return engine;
}

QStringList DataEngineManager::knownEngines()
{
    QStringList engines;
    KService::List offers = KServiceTypeTrader::self()->query("Amarok/DataEngine");
    foreach (KService::Ptr service, offers) {
        engines.append(service->property("X-EngineName").toString());
    }
    
    return engines;
} */

} // Context namespace

