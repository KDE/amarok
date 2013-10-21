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

#include <QHash>
#include <QObject>
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>

class QMetaEnum;

namespace AmarokScript
{
    template <class type, class WrapperType>
    void fromScriptValue( const QScriptValue &obj, type &object )
    {
        const WrapperType *wrapper = dynamic_cast<WrapperType*>( obj.toQObject() );
        if( wrapper )
            object = wrapper->data();
        else
            object = 0;
    }

    template <class type, class WrapperType>
    QScriptValue toScriptValue( QScriptEngine *engine, type const &object )
    {
        WrapperType *wrapper = new WrapperType( object );
        return engine->newQObject( wrapper, QScriptEngine::ScriptOwnership,
                                                QScriptEngine::ExcludeSuperClassContents );
    }

    template <class Container>
    QScriptValue toScriptArray( QScriptEngine *engine, const Container &container )
    {
        QScriptValue scriptArray = engine->newArray();
        typename Container::const_iterator begin = container.begin();
        typename Container::const_iterator end = container.end();
        typename Container::const_iterator it;
        for( it = begin; it != end; ++it )
            scriptArray.setProperty( quint32(it - begin), engine->toScriptValue(*it) );
        return scriptArray;
    }

    template <class Container>
    void fromScriptArray( const QScriptValue &value, Container &container )
    {
        quint32 len = value.property( "length" ).toUInt32();
        for( quint32 i = 0; i < len; ++i )
        {
            QScriptValue item = value.property( i );
            typedef typename Container::value_type ContainerValue;
            container.push_back( qscriptvalue_cast<ContainerValue>(item) );
        }
    }

    template <class Map>
    QScriptValue toScriptMap( QScriptEngine *engine, const Map &map )
    {
        QScriptValue scriptMap = engine->newObject();
        for( typename Map::const_iterator it( map.begin() ); it != map.end(); ++it )
            scriptMap.setProperty( it.key(), engine->toScriptValue( it.value() ) );
        return scriptMap;
    }

    template <class Map>
    void fromScriptMap( const QScriptValue &value, Map &map )
    {
        QScriptValueIterator it( value );
        while( it.hasNext() )
        {
            it.next();
            map[it.name()] = qscriptvalue_cast<typename Map::mapped_type>( it.value() );
        }
    }

   class AmarokScriptEngine : public QScriptEngine
    {
        Q_OBJECT

        public:
            AmarokScriptEngine( QObject *parent );
            virtual ~AmarokScriptEngine();

            void setDeprecatedProperty( const QString &parent, const QString &name, const QScriptValue &property );
            QString setUndocumentedProperty( const QString &name, const QScriptValue &property );
            // exposing the metaobject directly also exposes >900 other values
            QScriptValue enumObject( const QMetaEnum &metaEnum );

            template <class Pointer, class Wrapper>
            void registerPointerType()
            {
                 qScriptRegisterMetaType<Pointer>( this,
                                                   toScriptValue<Pointer, Wrapper>,
                                                   fromScriptValue<Pointer, Wrapper> );
            }

            template <class T>
            void registerArrayType()
            {
                qScriptRegisterMetaType<T>( this, toScriptArray, fromScriptArray );
            }
            template <class Map>
            void registerMapType()
            {
                qScriptRegisterMetaType<Map>( this, toScriptMap, fromScriptMap );
            }

        public slots:
            void slotDeprecatedCall( const QString &call );
            void setTimeout( const QScriptValue &value, int time );

        private slots:
            void slotTimeout();

        signals:
            void deprecatedCall(QString);
            void timeout( QScriptValue );

        private:
            const QString internalObject;
            QHash<QObject*, QScriptValue> m_callbacks;
    };
}

#endif // AMAROKSCRIPT_SCRIPTING_DEFINES_H
