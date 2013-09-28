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

#include <QtGui/QApplication>
#include <QtScript/QScriptEngine>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#define FAIL 0xA

int main(int argc, char **argv)
{
    QApplication app( argc, argv, false );

    QStringList allowedBindings;
    allowedBindings << "qt.core" << "qt.dbus" << "qt.gui" << "qt.network" << "qt.opengl"
                    << "qt.phonon" << "qt.sql" << "qt.svg" << "qt.uitools" << "qt.webkit"
                    << "qt.xml" << "qt.xmlpatterns";
    QScriptEngine engine;
    foreach( const QString &binding, allowedBindings )
    {
        QScriptValue error = engine.importExtension( binding );
        if( error.isUndefined() )
            continue; // undefined indicates success

        qDebug() << "Extension" << binding <<  "not found:" << error.toString();
        qDebug() << "Available extensions:" << engine.availableExtensions();
        return FAIL;
    }
    return 0;
}
