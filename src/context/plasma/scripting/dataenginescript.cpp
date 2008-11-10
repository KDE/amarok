/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
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

#include "dataenginescript.h"

#include "dataengine.h"

namespace Plasma
{

class DataEngineScriptPrivate
{
public:
    DataEngine *dataEngine;
};

DataEngineScript::DataEngineScript(QObject *parent)
    : ScriptEngine(parent),
      d(new DataEngineScriptPrivate)
{
}

DataEngineScript::~DataEngineScript()
{
//    delete d;
}

void DataEngineScript::setDataEngine(DataEngine *dataEngine)
{
    d->dataEngine = dataEngine;
}

DataEngine *DataEngineScript::dataEngine() const
{
    return d->dataEngine;
}

bool DataEngineScript::sourceRequestEvent(const QString &name)
{
    Q_UNUSED(name);
    return false;
}

bool DataEngineScript::updateSourceEvent(const QString &source)
{
    Q_UNUSED(source);
    return false;
}

void DataEngineScript::setData(const QString &source, const QString &key,
                               const QVariant &value)
{
    if (d->dataEngine) {
        d->dataEngine->setData(source, key, value);
    }
}

void DataEngineScript::setData(const QString &source, const QVariant &value)
{
    if (d->dataEngine) {
        d->dataEngine->setData(source, value);
    }
}

void DataEngineScript::removeAllData(const QString &source)
{
    if (d->dataEngine) {
        d->dataEngine->removeAllData(source);
    }
}

void DataEngineScript::removeData(const QString &source, const QString &key)
{
    if (d->dataEngine) {
        d->dataEngine->removeData(source, key);
    }
}

void DataEngineScript::setMaxSourceCount(uint limit)
{
    if (d->dataEngine) {
        d->dataEngine->setMaxSourceCount(limit);
    }
}

void DataEngineScript::setMinimumPollingInterval(int minimumMs)
{
    if (d->dataEngine) {
        d->dataEngine->setMinimumPollingInterval(minimumMs);
    }
}

int DataEngineScript::minimumPollingInterval() const
{
    if (d->dataEngine) {
        return d->dataEngine->minimumPollingInterval();
    }
    return 0;
}

void DataEngineScript::setPollingInterval(uint frequency)
{
    if (d->dataEngine) {
        d->dataEngine->setPollingInterval(frequency);
    }
}

void DataEngineScript::removeAllSources()
{
    if (d->dataEngine) {
        d->dataEngine->removeAllSources();
    }
}

} // Plasma namespace

#include "dataenginescript.moc"
