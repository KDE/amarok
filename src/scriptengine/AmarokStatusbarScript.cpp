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

#include "AmarokStatusbarScript.h"

#include "App.h"
#include "statusbar/StatusBar.h"


namespace AmarokScript
{
    AmarokStatusbarScript::AmarokStatusbarScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        Q_UNUSED( ScriptEngine );
    }

    AmarokStatusbarScript::~AmarokStatusbarScript()
    {

    }

   /* void AmarokStatusbarScript::setMainText( const QString &text )
    {
        The::statusBar()->setMainText( text );
    }

    void AmarokStatusbarScript::setMainTextIcon( QPixmap icon )
    {
        The::statusBar()->setMainTextIcon( icon );
    }

    void AmarokStatusbarScript::hideMainTextIcon()
    {
        The::statusBar()->hideMainTextIcon();
    }

    void AmarokStatusbarScript::resetMainText()
    {
        The::statusBar()->resetMainText();
    }*/

    void AmarokStatusbarScript::longMessage( const QString &text )
    {
        The::statusBar()->longMessage( text );
    }

   /* void AmarokStatusbarScript::shortLongMessage( const QString &_short, const QString &_long )
    {
        The::statusBar()->shortLongMessage( _short, _long );
    }*/

    void AmarokStatusbarScript::shortMessage( const QString &text )
    {
        The::statusBar()->shortMessage( text );
    }
}

#include "AmarokStatusbarScript.moc"

