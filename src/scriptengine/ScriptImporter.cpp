/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ScriptImporter.h"

#include "App.h"
#include "Debug.h"

#include <KUrl>

#include <QSet>

namespace AmarokScript
{
    ScriptImporter::ScriptImporter( QScriptEngine* scriptEngine, KUrl url )
    : QObject( kapp )
      , m_scriptUrl( url )
      , m_scriptEngine( scriptEngine )
    {  }

    ScriptImporter::~ScriptImporter()
    {
    }

    void
    ScriptImporter::loadExtension( const QString& src )
    {
        DEBUG_BLOCK

        m_scriptEngine->importExtension( "amarok/" + src );

    }

    bool
    ScriptImporter::loadQtBinding( const QString& binding )
    {
        DEBUG_BLOCK
        debug() << "importing qt bindings " << binding;
        QSet<QString> allowedBindings;
        allowedBindings << "qt.core" << "qt.gui" << "qt.sql" << "qt.webkit" << "qt.xml" << "qt.uitools" << "qt.network";
        if( allowedBindings.contains( binding ) )
        {
            if( !m_importedBindings.contains( binding ) )
            {
//I'm pretty sure this warning isn't true, the order doesn't appear to matter. - Ian
/*                if( ( binding != "qt.core" || binding != "qt.dbus" ) && ( !m_importedBindings.contains( "qt.core" ) ) )
                {
                    warning() << "qt.core should be included before the other bindings!";
                }*/
                if( m_scriptEngine->importExtension( binding ).isUndefined() )
                { // undefined indiciates success
                    m_importedBindings << binding;
                    return true;
                }
                //else fall through and return false
            }
            else
                return true;
        }
        else
            warning() <<"Qt Binding: " << binding << " not found!";
        return false;
    }

    bool
    ScriptImporter::include( const QString& relativeFilename )
    {
        KUrl includeUrl = m_scriptUrl.upUrl();
        includeUrl.addPath( relativeFilename );
        QFile file( includeUrl.toLocalFile() );
        if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            warning() << "cannot open the include file!";
            return false;
        }
        m_scriptEngine->currentContext()->setActivationObject(
                              m_scriptEngine->currentContext()->parentContext()->activationObject() );
        m_scriptEngine->evaluate( file.readAll(), relativeFilename );
        return true;
    }
}
#include "ScriptImporter.moc"

