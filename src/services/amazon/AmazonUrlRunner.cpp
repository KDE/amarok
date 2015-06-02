/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#include "AmazonShoppingCart.h"
#include "AmazonUrlRunner.h"

#include <KLocale>

AmazonUrlRunner::AmazonUrlRunner()
 : QObject()
 , AmarokUrlRunnerBase()
{
}

AmazonUrlRunner::~AmazonUrlRunner()
{
}

QString
AmazonUrlRunner::command() const
{
    return "service-amazonstore";
}

QString
AmazonUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that triggers an action in the integrated MP3 Music Store service", "Amazon" );
}

QIcon
AmazonUrlRunner::icon() const
{
    return QIcon::fromTheme( "view-services-amazon-amarok" );
}

bool
AmazonUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK
    if( !url.isNull() )
    {
        QString command = url.args().value( "command" );

        if( command == "search" )
        {
            QString request = url.args().value( "filter" );
            emit( search( request ) );
        }
        else if( command == "addToCart")
        {
            QString asin = url.args().value( "asin" );
            QString name = url.args().value( "name" );
            QString price = url.args().value( "price" );

            // do nothing if url is invalid
            if( asin.isEmpty() || name.isEmpty() || price.isEmpty() )
                return false;
            else
                AmazonShoppingCart::instance()->add( asin, price, name );
        }
    }
    return true;
}


