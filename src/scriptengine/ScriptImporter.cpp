/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "ScriptImporter.h"

#include "App.h"
#include "Debug.h"

#include <KUrl>

#include <QFileInfo>
#include <QtScript>

namespace Amarok
{
    ScriptImporter::ScriptImporter( QScriptEngine* ScriptEngine, KUrl url )
    : QObject( kapp )
    {
        m_URL = url;
        m_ScriptEngine = ScriptEngine;
    }

    ScriptImporter::~ScriptImporter()
    {
    }

    void ScriptImporter::load( QString src )
    {
        DEBUG_BLOCK

        QFileInfo info( m_URL.path() );
        src = info.path() + '/' + src;
        debug() << src ;
        QFile scriptFile( src );
        scriptFile.open( QIODevice::ReadOnly );
        m_ScriptEngine->evaluate( scriptFile.readAll() );
        scriptFile.close();
    }

    void ScriptImporter::loadQtBinding( QString binding )
    {
        DEBUG_BLOCK
        debug() << "importing qt bindings...";
        if ( binding == "qt.core" )
            m_ScriptEngine->importExtension( "qt.core" );
        else if ( binding == "qt.gui" )
            m_ScriptEngine->importExtension( "qt.gui" );
        else if ( binding == "qt.sql" )
            m_ScriptEngine->importExtension( "qt.sql" );
        else if ( binding == "qt.webkit" )
            m_ScriptEngine->importExtension( "qt.webkit" );
        else if ( binding == "qt.xml" )
            m_ScriptEngine->importExtension( "qt.xml" );
        else if ( binding == "qt.uitools" )
            m_ScriptEngine->importExtension( "qt.uitools" );
        else
            warning() <<"Qt Binding: "<<binding<<" not found!";
    }
}

#include "ScriptImporter.moc"
