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
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>

class QMetaEnum;

namespace AmarokScript
{
    template <class type, class WrapperType>
    void fromScriptValue( const QJSValue &obj, type &object )
    {
        const WrapperType *wrapper = dynamic_cast<WrapperType*>( obj.toQObject() );
        if( wrapper )
            object = wrapper->data();
        else
            object = nullptr;
    }

    template <class type, class WrapperType>
    QJSValue toScriptValue( QJSEngine *engine, type const &object )
    {
        WrapperType *wrapper = new WrapperType( object );
        return engine->newQObject( wrapper );
    }

    template <class Container>
    QJSValue toScriptArray( QJSEngine *engine, const Container &container )
    {
        QJSValue scriptArray = engine->newArray();
        typename Container::const_iterator begin = container.begin();
        typename Container::const_iterator end = container.end();
        typename Container::const_iterator it;
        for( it = begin; it != end; ++it )
            scriptArray.setProperty( quint32(it - begin), engine->toScriptValue(*it) );
        return scriptArray;
    }

    template <class Container>
    void fromScriptArray( const QJSValue &value, Container &container )
    {
        quint32 len = value.property( QStringLiteral("length") ).toUInt();
        for( quint32 i = 0; i < len; ++i )
        {
            QJSValue item = value.property( i );
            typedef typename Container::value_type ContainerValue;
            container.push_back( qjsvalue_cast<ContainerValue>(item) );
        }
    }

    template <class Map>
    QJSValue toScriptMap( QJSEngine *engine, const Map &map )
    {
        QJSValue scriptMap = engine->newObject();
        for( typename Map::const_iterator it( map.begin() ); it != map.end(); ++it )
            scriptMap.setProperty( it.key(), engine->toScriptValue( it.value() ) );
        return scriptMap;
    }

    template <class Map>
    void fromScriptMap( const QJSValue &value, Map &map )
    {
        QJSValueIterator it( value );
        while( it.hasNext() )
        {
            it.next();
            map[it.name()] = qjsvalue_cast<typename Map::mapped_type>( it.value() );
        }
    }

    /**
     * SCRIPTDOX _
     */
    class AmarokScriptEngine : public QJSEngine
    {
        Q_OBJECT

        public:
            explicit AmarokScriptEngine( QObject *parent );
            ~AmarokScriptEngine() override;

            void setDeprecatedProperty( const QString &parent, const QString &name, const QJSValue &property );
            // exposing the metaobject directly also exposes >900 other values
            QJSValue enumObject( const QMetaEnum &metaEnum );

            template <class T>
            void registerArrayType()
            {
                qRegisterMetaType<T>();
                QMetaType::registerConverter<QJSValue,T>( [] (QJSValue scriptObj) {
                    T arrayObj;
                    fromScriptArray( scriptObj, arrayObj );
                    return arrayObj;
                });
                QMetaType::registerConverter<T,QJSValue>( [this] (T arrayObj) { return toScriptArray( this, arrayObj ); } );
            }
            template <class Map>
            void registerMapType()
            {
                qRegisterMetaType<Map>();
                QMetaType::registerConverter<QJSValue,Map>( [] (QJSValue scriptObj) {
                    Map mapObj;
                    fromScriptMap( scriptObj, mapObj );
                    return mapObj;
                });
                QMetaType::registerConverter<Map,QJSValue>( [this] (Map mapObj) { return toScriptMap( this, mapObj ); } );
            }

            // SCRIPTDOX exclude
            Q_INVOKABLE void invokableDeprecatedCall( const QString &call );

            /**
             * @param function The function to invoke after time @param time in milliseconds.
             * @param thisObject [Optional] The this object this function is invoked with.
             * @param args [Optional] An array containing arguments this function is to be invoked with.
             */
            Q_INVOKABLE void setTimeout( const QJSValue &function, int time,
                             const QJSValue &thisObject = QJSValue(),
                             const QJSValue &args = QJSValue() );

        private Q_SLOTS:
            void slotTimeout();

        Q_SIGNALS:
            void deprecatedCall(const QString &);

        private:
            const QString internalObject;
            QHash<QObject*, QJSValueList> m_callbacks;
    };
}

#endif // AMAROKSCRIPT_SCRIPTING_DEFINES_H
