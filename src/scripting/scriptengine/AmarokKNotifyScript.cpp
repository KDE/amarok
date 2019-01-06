/****************************************************************************************
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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

#include "AmarokKNotifyScript.h"

#include "amarokconfig.h"
#include "App.h"
#include "KNotificationBackend.h"

#include <QScriptEngine>

#define kNotify Amarok::KNotificationBackend::instance()

using namespace AmarokScript;

AmarokKNotifyScript::AmarokKNotifyScript( QScriptEngine *engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    QScriptValue windowObject = engine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Window") );
    windowObject.setProperty( QStringLiteral("KNotify"), scriptObject );
}

bool
AmarokKNotifyScript::kNotifyEnabled()
{
    return AmarokConfig::kNotifyEnabled();
}

void
AmarokKNotifyScript::setKNotifyEnabled( bool enable )
{
    AmarokConfig::setKNotifyEnabled( enable );
}

void
AmarokKNotifyScript::show( const QString &title, const QString &body, const QPixmap &pixmap )
{
    kNotify->show( title, body, pixmap );
}


void
AmarokKNotifyScript::showCurrentTrack()
{
    kNotify->showCurrentTrack( true );
}
