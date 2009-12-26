/*
 * Copyright (C) 2009 Ian Monroe <ian@monroe.nu>
 * released under public domain or:
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <QtCore/QCoreApplication>
#include <QtScript/QScriptEngine>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#define FAIL 0xA

int main(int argc, char **argv)
{
    QCoreApplication app( argc, argv );

    QStringList allowedBindings;
    allowedBindings << "qt.core" << "qt.gui" << "qt.sql" << "qt.xml" << "qt.uitools" << "qt.network";
    QScriptEngine engine;
    foreach( QString binding, allowedBindings )
    {
        QScriptValue error = engine.importExtension( binding );
        if( error.isUndefined() )
        { // undefined indiciates success
            continue;
        }

        qDebug() << "Extension" << binding <<  "not found:" << error.toString();
        qDebug() << "Available extensions:" << engine.availableExtensions();
        return FAIL;
    }
    return 0;
}
