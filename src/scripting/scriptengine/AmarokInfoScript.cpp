/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AmarokInfoScript.h"

#include "core/support/Amarok.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <KIconLoader>

#include <QMetaEnum>

using namespace AmarokScript;

InfoScript::InfoScript( const QUrl &scriptUrl, AmarokScriptEngine *engine )
    : QObject( engine )
    , m_scriptUrl( scriptUrl )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Info"), scriptObject );

    const QMetaEnum iconEnum = metaObject()->enumerator( metaObject()->indexOfEnumerator("IconSizes") );
    Q_ASSERT( iconEnum.isValid() );
    scriptObject.setProperty( QStringLiteral("IconSizes"), engine->enumObject( iconEnum ) );
}

QString
InfoScript::version() const
{
    return QStringLiteral(AMAROK_VERSION);
}

QString
InfoScript::scriptPath() const
{
    return m_scriptUrl.adjusted(QUrl::RemoveFilename).path();
}

QString
InfoScript::iconPath( const QString& name, int size ) const
{
    //if size was positive it would refer to KIconLoader::Group
    return KIconLoader::global()->iconPath( name, -size );
}

QString
InfoScript::scriptConfigPath( const QString& name ) const
{
    return Amarok::saveLocation( QStringLiteral("scripts/") + name );
}
