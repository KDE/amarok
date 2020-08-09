/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QTBINDING_H
#define QTBINDING_H

#include <QJSEngine>
#include <QMetaMethod>
#include <QRegularExpression>
#include <QSet>

namespace QtBindings {
    /** Base template for QT bindings
     * IMPORTANT: for methods do be correctly exported, static methods must be
     * annotated with Q_INVOKABLE, and object methods must be declared as public slots
     * */
    template<class T> class Base
    {
    public:
        static void installJSType( QJSEngine *engine )
        {
            if (!engine) return;

           const QByteArray typeName = QString( T::staticMetaObject.className() )
                            .remove(  QRegularExpression( "^.*::" ) ).toLatin1();
           const QByteArray typeNamePtr = typeName + "*";
           const QByteArray typeNameRef = typeName + "&";
           const QString qTypeName( "Q" + typeName );

            // Install type only once along program execution
            if ( !QMetaType::isRegistered( QMetaType::type( typeName ) ) ) {
                qRegisterMetaType<T>( typeName );
                qRegisterMetaType<T>( typeNameRef );
                qRegisterMetaType<T*>( typeNamePtr );
                qRegisterMetaType<T>(  "const " + typeName );
                qRegisterMetaType<T>(  "const " + typeNameRef );
                qRegisterMetaType<T*>( "const " + typeNamePtr );

                /* Converter allows passing parameters to C++ via value and const-ref */
                bool conv = QMetaType::registerConverter<QObject*,T>( [] (QObject* qObjPtr) {
                    const T* dataPtr = qobject_cast<T*>( qObjPtr );
                    return (dataPtr == nullptr) ? T() : T( *dataPtr ) ;
                });
                Q_ASSERT(conv);
            }

            // Export type to each JS engine only once
            if ( engine->globalObject().property( qTypeName ).isUndefined() ) {
                engine->globalObject().setProperty( qTypeName, engine->newQMetaObject<T>());

                QJSValue classObj = engine->newQObject( new T() );
                // Add static methods to the associated JS object
                for ( const QString& methodName : getStaticMethods() ) {
                    engine->globalObject().property( qTypeName ).setProperty( methodName,
                            classObj.property( methodName ) );
                }
            }
        }

        static QSet<QString> getStaticMethods()
        {
            const QMetaObject classObj = T::staticMetaObject;
            QSet<QString> methodList;
            for (int i = classObj.methodOffset(); i < classObj.methodCount(); i++) {
                /* TODO - non-static methods are filtered out by being slots since
                 * tags do not work for Q_INVOKABLE. Change this to tags if fixed */
                if (classObj.method(i).methodType() == QMetaMethod::Method)
                    methodList << classObj.method(i).name();
            }
            return methodList;
        }
    };
}

#endif //QTBINDING_H
