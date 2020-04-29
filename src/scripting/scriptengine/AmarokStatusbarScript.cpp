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

#include "core/logger/Logger.h"
#include "core/support/Components.h"

#include <QJSEngine>

using namespace AmarokScript;

AmarokStatusbarScript::AmarokStatusbarScript( QJSEngine *engine )
    : QObject( engine )
{
    QJSValue scriptObject = engine->newQObject( this, QJSEngine::AutoOwnership,
                                                    QJSEngine::ExcludeSuperClassContents );
    QJSValue windowObject = engine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Window") );
    windowObject.setProperty( QStringLiteral("Statusbar"), scriptObject );
}

void
AmarokStatusbarScript::longMessage( const QString &text )
{
    Amarok::Logger::longMessage( text );
}

void
AmarokStatusbarScript::shortMessage( const QString &text )
{
    Amarok::Logger::shortMessage( text );
}
