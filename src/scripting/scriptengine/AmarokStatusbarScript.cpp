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

#include "AmarokStatusbarScript.h"

#include "core/interfaces/Logger.h"
#include "core/support/Components.h"

#include <QScriptEngine>

using namespace AmarokScript;

AmarokStatusbarScript::AmarokStatusbarScript( QScriptEngine *engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    QScriptValue windowObject = engine->globalObject().property( "Amarok" ).property( "Window" );
    windowObject.setProperty( "Statusbar", scriptObject );
}

void
AmarokStatusbarScript::longMessage( const QString &text )
{
    Amarok::Components::logger()->longMessage( text );
}

void
AmarokStatusbarScript::shortMessage( const QString &text )
{
    Amarok::Components::logger()->shortMessage( text );
}
