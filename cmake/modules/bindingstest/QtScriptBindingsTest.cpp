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

#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KCmdLineArgs>
#include <QtScript/QScriptEngine>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#define FAIL 0xA

int main(int argc, char **argv)
{
    KAboutData about( "findgeneratorplugins", "", ki18n("Generator Exists?"), "1.0", ki18n("Find if the QtScript Plugins Are Installed"), KAboutData::License_LGPL_V2 );
    KCmdLineArgs::init( argc, argv, &about );
    KApplication app( false ); //no gui pls
    
    QStringList allowedBindings;
    allowedBindings << "qt.core" << "qt.gui" << "qt.sql" << "qt.xml" << "qt.uitools" << "qt.network";
    QScriptEngine engine;
    foreach( QString binding, allowedBindings )
    {
        if( !engine.importExtension( binding ).isUndefined() )
        { // undefined indiciates success
            qDebug() << binding <<  " not found";
            return FAIL;
        }
    }
    return 0;
}
