/****************************************************************************************
 * Copyright (c) 2007 Richard J. Moore <rich@kde.org>                                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BIND_DATAENGINE_H
#define BIND_DATAENGINE_H

#include <QtScript/QtScript>
#include <KDebug>

#include <Plasma/DataEngine>
#include <Plasma/Service>
#include <Plasma/ServiceJob>

using namespace Plasma;

#if !KDE_IS_VERSION(4, 4, 50)
Q_DECLARE_METATYPE(DataEngine*)
#endif
Q_DECLARE_METATYPE(Service*)
Q_DECLARE_METATYPE(ServiceJob*)
Q_DECLARE_METATYPE(QVariant)
Q_DECLARE_METATYPE(DataEngine::Dict)
Q_DECLARE_METATYPE(DataEngine::Data)


template <class M>
QScriptValue qScriptValueFromMap(QScriptEngine *eng, const M &map)
{
    kDebug() << "qScriptValueFromMap called";

    QScriptValue obj = eng->newObject();
    typename M::const_iterator begin = map.constBegin();
    typename M::const_iterator end = map.constEnd();
    typename M::const_iterator it;
    for (it = begin; it != end; ++it)
        obj.setProperty(it.key(), qScriptValueFromValue(eng, it.value()));
    return obj;
}

template <class M>
void qScriptValueToMap(const QScriptValue &value, M &map)
{
    QScriptValueIterator it(value);
    while (it.hasNext()) {
        it.next();
        map[it.name()] = qscriptvalue_cast<typename M::mapped_type>(it.value());
    }
}

template<typename T>
int qScriptRegisterMapMetaType(
    QScriptEngine *engine,
    const QScriptValue &prototype = QScriptValue()
#ifndef qdoc
    , T * /* dummy */ = 0
#endif
)
{
    return qScriptRegisterMetaType<T>(engine, qScriptValueFromMap,
                                      qScriptValueToMap, prototype);
}

#endif // BIND_DATAENGINE_H

