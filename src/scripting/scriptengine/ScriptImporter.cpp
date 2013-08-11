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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ScriptImporter.h"

#include "App.h"
#include <config.h>
#include "core/support/Debug.h"

#include <KUrl>

#include <QScriptEngine>
#include <QSet>

using namespace AmarokScript;

ScriptImporter::ScriptImporter( QScriptEngine* scriptEngine, const KUrl &url )
    : QObject( scriptEngine )
    , m_scriptUrl( url )
    , m_scriptEngine( scriptEngine )
{
    QScriptValue scriptObject = scriptEngine->newQObject( this, QScriptEngine::AutoOwnership
                                                        , QScriptEngine::ExcludeSuperClassContents );
    scriptEngine->globalObject().setProperty( "Importer", scriptObject );
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
    QStringList availableBindings = m_scriptEngine->availableExtensions();
    if( availableBindings.contains( binding ) )
    {
        if( !m_importedBindings.contains( binding ) )
        {
            if( m_scriptEngine->importExtension( binding ).isUndefined() )
            { // undefined indicates success
                m_importedBindings << binding;
                return true;
            }
            //else fall through and return false
        }
        else
            return true;
    }
    else
        warning() << __PRETTY_FUNCTION__ << "Binding \"" << binding << "\" could not be found!";
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
        // m_scriptEngine->currentContext()->throwError( QScriptContext::TypeError, "Include file could not be opened!" );
        return false;
    }
    m_scriptEngine->currentContext()->setActivationObject(
                            m_scriptEngine->currentContext()->parentContext()->activationObject() );
    m_scriptEngine->evaluate( file.readAll(), relativeFilename );
    return true;
}

QStringList
ScriptImporter::availableBindings() const
{
    return m_scriptEngine->availableExtensions();
}
