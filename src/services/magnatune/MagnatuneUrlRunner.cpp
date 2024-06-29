/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "MagnatuneUrlRunner.h"

#include <KLocalizedString>

MagnatuneUrlRunner::MagnatuneUrlRunner()
 : QObject()
 , AmarokUrlRunnerBase()
{
}

MagnatuneUrlRunner::~MagnatuneUrlRunner()
{
}

QString MagnatuneUrlRunner::command() const
{
    return QStringLiteral("service-magnatune");
}

QString MagnatuneUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that triggers an action in the integrated Magnatune.com service", "Magnatune" );
}

QIcon MagnatuneUrlRunner::icon() const
{
    return QIcon::fromTheme( QStringLiteral("view-services-magnatune-amarok") );
}

bool MagnatuneUrlRunner::run( const AmarokUrl &url )
{
    DEBUG_BLOCK
    if ( !url.isNull() )
    {
        QString command = url.args().value( QStringLiteral("command") );

        if( command == QStringLiteral("show_favorites") )
        {
            Q_EMIT( showFavorites() );
        }
        else if ( command == QStringLiteral("show_home") )
        {
            Q_EMIT( showHome() );
        }
        else if ( command == QStringLiteral("show_recommendations") )
        {
            Q_EMIT( showRecommendations() );
        }
        else if ( command == QStringLiteral("download") || command == QStringLiteral("purchase") || command == QStringLiteral("buy") )
        {
            //allow some aliases for this command as the context might make one of
            //them more appropriate. In any case, non and stream  members will be given the
            //purchase dialog and will have to pay, download members will get the
            //free download

            if ( url.args().keys().contains( QStringLiteral("sku") ) )
            {
                QString sku = url.args().value( QStringLiteral("sku") );
                Q_EMIT( buyOrDownload( sku ) );
            }
        }
        else if ( command == QStringLiteral("remove_favorite") )
        {
            if ( url.args().keys().contains( QStringLiteral("sku") ) )
            {
                QString sku = url.args().value( QStringLiteral("sku") );
                debug() << "remove from favorites sku: " << sku;
                Q_EMIT( removeFromFavorites( sku ) );
            }
        }

    }
    return true;
}


