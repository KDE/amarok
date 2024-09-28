/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "AmarokScript.h"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "scripting/scriptmanager/ScriptManager.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QJSEngine>

AmarokScript::AmarokScript::AmarokScript( const QString &name, QJSEngine *engine )
    : QObject( engine )
    , m_name( name )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().setProperty( QStringLiteral("Amarok"), scriptObject );
    if( ScriptManager::instance()->m_scripts.contains( name ) )
        connect( ScriptManager::instance()->m_scripts[name], &ScriptItem::uninstalled, this, &AmarokScript::uninstalled );
}

void
AmarokScript::AmarokScript::quitAmarok()
{
    pApp->quit();
}

void
AmarokScript::AmarokScript::debug( const QString& text ) const
{
    ::debug() << "SCRIPT" << m_name << ": " << text;
}

int
AmarokScript::AmarokScript::alert( const QString& text, const QString& type ) const
{
    //Ok = 1, Cancel = 2, Yes = 3, No = 4, Continue = 5
    if( type == QLatin1String("error") )
    {
        KMessageBox::error( nullptr, text );
        return -1;
    }
    else if( type == QLatin1String("information") )
    {
        KMessageBox::information( nullptr, text );
        return -1;
    }
    else if( type == QLatin1String("questionYesNo") )
        return KMessageBox::questionTwoActions( nullptr, text, text,
            KGuiItem( i18nc( "Generic script dialog answer button", "Yes") ),
            KGuiItem( i18nc( "Generic script dialog answer button", "No") ) );
    else if( type == QLatin1String("questionYesNoCancel") )
        return KMessageBox::questionTwoActionsCancel( nullptr, text, text,
            KGuiItem( i18nc( "Generic script dialog answer button", "Yes") ),
            KGuiItem( i18nc( "Generic script dialog answer button", "No") ) );
    else if( type == QLatin1String("warningYesNo") )
        return KMessageBox::warningTwoActions( nullptr, text, text,
            KGuiItem( i18nc( "Generic script dialog answer button", "Yes") ),
            KGuiItem( i18nc( "Generic script dialog answer button", "No") ) );
    else if( type == QLatin1String("warningContinueCancel") )
        return KMessageBox::warningContinueCancel( nullptr, text );
    else if( type == QLatin1String("warningYesNoCancel") )
        return KMessageBox::warningTwoActionsCancel( nullptr, text, text,
            KGuiItem( i18nc( "Generic script dialog answer button", "Yes") ),
            KGuiItem( i18nc( "Generic script dialog answer button", "No") ) );

    debug( QStringLiteral("alert type not found!") );
    return -1;
}

void
AmarokScript::AmarokScript::end()
{
    ScriptManager::instance()->stopScript( m_name );
}

bool
AmarokScript::AmarokScript::runScript( const QString& name ) const
{
    return ScriptManager::instance()->runScript( name, true );
}

bool
AmarokScript::AmarokScript::stopScript( const QString& name ) const
{
    return ScriptManager::instance()->stopScript( name );
}

QStringList
AmarokScript::AmarokScript::listRunningScripts() const
{
    return ScriptManager::instance()->listRunningScripts();
}
