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

#include <QObject>
#include <QScriptValue>
#include <QScriptEngine>

namespace AmarokScript
{
    class AmarokScriptEngine : public QScriptEngine
    {
        Q_OBJECT

        public:
            AmarokScriptEngine( QObject *parent );
            virtual ~AmarokScriptEngine();

            void setDeprecatedProperty( const QString &parent, const QString &name, const QScriptValue &property );
            QString setUndocumentedProperty( const QString &name, const QScriptValue &property );;

        private:
            const QString internalObject;

        public slots:
            void slotDeprecatedCall( const QString &call );

        signals:
            void deprecatedCall(QString);
    };

    template <class Container>
    QScriptValue toScriptArray( QScriptEngine *engine, const Container &container )
    {
        QScriptValue scriptArray = engine->newArray();
        typename Container::const_iterator begin = container.begin();
        typename Container::const_iterator end = container.end();
        typename Container::const_iterator it;
        for (it = begin; it != end; ++it)
            scriptArray.setProperty(quint32(it - begin), engine->toScriptValue(*it));
        return scriptArray;
    }

    template <class Container>
    void fromScriptArray( const QScriptValue &value, Container &container )
    {
        quint32 len = value.property("length").toUInt32();
        for (quint32 i = 0; i < len; ++i) {
            QScriptValue item = value.property(i);
            typedef typename Container::value_type ContainerValue;
            container.push_back(qscriptvalue_cast<ContainerValue>(item));
        }
    }
}

#endif // AMAROKSCRIPT_SCRIPTING_DEFINES_H
