/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "dataenginemanager.h"

#include <KDebug>
#include <KServiceTypeTrader>

#include "private/dataengine_p.h"
#include "scripting/scriptengine.h"

namespace Plasma
{

class NullEngine : public DataEngine
{
    public:
        NullEngine(QObject *parent = 0)
            : DataEngine(parent)
        {
            setValid(false);

            // ref() ourselves to ensure we never get deleted
            d->ref();
        }
};

class DataEngineManagerPrivate
{
    public:
        DataEngineManagerPrivate()
            : nullEng(0)
        {}

        ~DataEngineManagerPrivate()
        {
            foreach (Plasma::DataEngine *engine, engines) {
                delete engine;
            }
            engines.clear();
            delete nullEng;
        }

        DataEngine *nullEngine()
        {
            if (!nullEng) {
                nullEng = new NullEngine;
            }

            return nullEng;
        }

        DataEngine::Dict engines;
        DataEngine *nullEng;
};

class DataEngineManagerSingleton
{
    public:
        DataEngineManager self;
};

K_GLOBAL_STATIC(DataEngineManagerSingleton, privateDataEngineManagerSelf)

DataEngineManager *DataEngineManager::self()
{
    return &privateDataEngineManagerSelf->self;
}

DataEngineManager::DataEngineManager()
    : d(new DataEngineManagerPrivate)
{
}

DataEngineManager::~DataEngineManager()
{
    delete d;
}

Plasma::DataEngine *DataEngineManager::engine(const QString &name) const
{
    Plasma::DataEngine::Dict::const_iterator it = d->engines.find(name);
    if (it != d->engines.end()) {
        // ref and return the engine
        //Plasma::DataEngine *engine = *it;
        return *it;
    }

    return d->nullEngine();
}

Plasma::DataEngine *DataEngineManager::loadEngine(const QString &name)
{
    Plasma::DataEngine *engine = 0;
    Plasma::DataEngine::Dict::const_iterator it = d->engines.find(name);

    if (it != d->engines.end()) {
        engine = *it;
        engine->d->ref();
        return engine;
    }

    // load the engine, add it to the engines
    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(name);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/DataEngine",
                                                              constraint);
    QString error;

    if (offers.isEmpty()) {
        kDebug() << "offers are empty for " << name << " with constraint " << constraint;
    } else {
        QVariantList allArgs;
        allArgs << offers.first()->storageId();
        QString api = offers.first()->property("X-Plasma-API").toString();
        if (api.isEmpty()) {
            if (offers.first()) {
                KPluginLoader plugin(*offers.first());
                if (Plasma::isPluginVersionCompatible(plugin.pluginVersion())) {
                    engine = offers.first()->createInstance<Plasma::DataEngine>(0, allArgs, &error);
                }
            }
        } else {
            engine = new DataEngine(0, offers.first());
        }
    }

    if (!engine) {
        kDebug() << "Couldn't load engine \"" << name << "\". Error given: " << error;
        return d->nullEngine();
    }

    engine->init();
    d->engines[name] = engine;
    return engine;
}

void DataEngineManager::unloadEngine(const QString &name)
{
    Plasma::DataEngine::Dict::iterator it = d->engines.find(name);

    if (it != d->engines.end()) {
        Plasma::DataEngine *engine = *it;
        engine->d->deref();

        if (!engine->d->isUsed()) {
            d->engines.erase(it);
            delete engine;
        }
    }
}

QStringList DataEngineManager::listAllEngines()
{
    QStringList engines;
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/DataEngine");
    foreach (const KService::Ptr &service, offers) {
        QString name = service->property("X-KDE-PluginInfo-Name").toString();
        if (!name.isEmpty()) {
            engines.append(name);
        }
    }

    return engines;
}

} // namespace Plasma

#include "dataenginemanager.moc"
