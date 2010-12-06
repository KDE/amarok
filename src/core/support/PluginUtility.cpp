/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "PluginUtility"

#include "core/support/PluginUtility.h"

#include "core/support/Debug.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QFile>

KService::List
Plugins::PluginUtility::query( const QString &constraint )
{
    // Add versioning constraint
    QString str = QString::fromLatin1( "[X-KDE-Amarok-framework-version] == %1" )
            .arg( Plugins::PluginFrameworkVersion );
    if( !constraint.trimmed().isEmpty() )
        str += QString::fromLatin1( " and %1" ).arg( constraint );
    str += QString::fromLatin1( " and [X-KDE-Amarok-rank] > 0" );

    debug() << "Plugin trader constraint:" << str;
    return KServiceTypeTrader::self()->query( "Amarok/Plugin", str );
}

void
Plugins::PluginUtility::showAbout( const QString &constraint )
{
    KService::List offers = query( constraint );

    if ( offers.isEmpty() )
        return;

    KService::Ptr s = offers.front();

    const QString body = "<tr><td>%1</td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"1\">";

    str += body.arg( i18nc( "Title, as in: the title of this item", "Name" ),                s->name() );
    str += body.arg( i18n( "Library" ),             s->library() );
    str += body.arg( i18n( "Authors" ),             s->property( "X-KDE-Amarok-authors" ).toStringList().join( "\n" ) );
    str += body.arg( i18nc( "Property, belonging to the author of this item", "Email" ),               s->property( "X-KDE-Amarok-email" ).toStringList().join( "\n" ) );
    str += body.arg( i18n( "Version" ),             s->property( "X-KDE-Amarok-version" ).toString() );
    str += body.arg( i18n( "Framework Version" ),   s->property( "X-KDE-Amarok-framework-version" ).toString() );

    str += "</table></body></html>";

    KMessageBox::information( 0, str, i18n( "Plugin Information" ) );
}

void
Plugins::PluginUtility::dump( const KService::Ptr service )
{
    #define ENDLI endl << Debug::indent()

    debug()
      << ENDLI
      << "PluginUtility Service Info:" << ENDLI
      << "---------------------------" << ENDLI
      << "name                          :" << service->name() << ENDLI
      << "library                       :" << service->library() << ENDLI
      << "desktopEntryPath              :" << service->entryPath() << ENDLI
      << "X-KDE-Amarok-plugintype       :" << service->property( "X-KDE-Amarok-plugintype" ).toString() << ENDLI
      << "X-KDE-Amarok-name             :" << service->property( "X-KDE-Amarok-name" ).toString() << ENDLI
      << "X-KDE-Amarok-authors          :" << service->property( "X-KDE-Amarok-authors" ).toStringList() << ENDLI
      << "X-KDE-Amarok-rank             :" << service->property( "X-KDE-Amarok-rank" ).toString() << ENDLI
      << "X-KDE-Amarok-version          :" << service->property( "X-KDE-Amarok-version" ).toString() << ENDLI
      << "X-KDE-Amarok-framework-version:" << service->property( "X-KDE-Amarok-framework-version" ).toString()
      << endl
     ;

    #undef ENDLI
}
