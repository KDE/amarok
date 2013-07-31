/****************************************************************************************
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

/****************************************************************************************
 * Basic settings for the MP3 Music Store API.                                          *
 * Contact Sven Krohlas <sven@asbest-online.de> if you want an API key to use this in   *
 * your project.                                                                        *
 ****************************************************************************************/

#ifndef AMAZON_H
#define AMAZON_H

#include "AmazonConfig.h"

#include <QString>
#include <QUrl>

#include <KGlobal>
#include <KLocale>

// API documentation: http://www.mp3-music-store.de/api.html
#define MP3_MUSIC_STORE_HOST "http://www.mp3-music-store.de"
#define MP3_MUSIC_STORE_KEY "27274503cb405cb1929f353fc507f09c"

// Example search:
// http://www.mp3-music-store.de/?apikey=27274503cb405cb1929f353fc507f09c&method=Search&Player=amarok&Location=de&Text=Shearer

namespace Amazon
{
/**
* Returns a valid URL that upon opening sets a cookie indicating that
* the user has a .amz downloader installed. This simplifies the checkout
* process a lot.
*/
inline QUrl
createCookieUrl()
{
    return QUrl( "http://www.amazon." + AmazonConfig::instance()->country() + "/gp/dmusic/after_download_manager_install.html?AMDVersion=1.0.9" );
}

/**
 * Returns the human readable pretty price for a given price.
 * The pretty price includes the currency symbol at a position
 * according to the current KDE locale.
 */
inline QString
prettyPrice( QString price )
{
    if( price.toInt() == 0 )
        return price;

    QString country = AmazonConfig::instance()->country();
    QString value;

    // determine human readable value
    if( country == "com" || country == "co.uk" || country == "de" || country == "es" || country == "fr" || country == "it" )
        value.setNum( price.toDouble() / 100 );
    else if( country == "co.jp" )
        value.setNum( price.toInt() );

    if( country == "de" || country == "es" || country == "fr" || country == "it" )
        return KGlobal::locale()->formatMoney( value.toFloat(), "€" , 2 );
    if( country == "co.uk" )
        return KGlobal::locale()->formatMoney( value.toFloat(), "£" , 2 );
    if( country == "com" )
        return KGlobal::locale()->formatMoney( value.toFloat(), "$" , 2 );
    if( country == "co.jp" )
        return KGlobal::locale()->formatMoney( value.toFloat(), "¥" , 0 );

    return QString();
}
}

#endif // AMAZON_H
