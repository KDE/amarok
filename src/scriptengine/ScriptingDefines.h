/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef AMAROKSCRIPT_SCRIPTING_DEFINES_H
#define AMAROKSCRIPT_SCRIPTING_DEFINES_H

#include <QScriptValue>
#include <QScriptEngine>

namespace AmarokScript
{
    //from qt docs, any issues?
    template <class Container>
    QScriptValue toScriptArray( QScriptEngine *engine, const Container &cont )
    {
        QScriptValue a = engine->newArray();
        typename Container::const_iterator begin = cont.begin();
        typename Container::const_iterator end = cont.end();
        typename Container::const_iterator it;
        for (it = begin; it != end; ++it)
            a.setProperty(quint32(it - begin), engine->toScriptValue(*it));
        return a;
    }

    template <class Container>
    void fromScriptArray( const QScriptValue &value, Container &cont )
    {
        quint32 len = value.property("length").toUInt32();
        for (quint32 i = 0; i < len; ++i) {
            QScriptValue item = value.property(i);
            typedef typename Container::value_type ContainerValue;
            cont.push_back(qscriptvalue_cast<ContainerValue>(item));
        }
    }
}

#endif // AMAROKSCRIPT_SCRIPTING_DEFINES_H
