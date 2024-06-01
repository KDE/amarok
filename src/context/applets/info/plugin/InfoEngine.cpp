/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#include "InfoEngine.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "browsers/InfoProxy.h"
#include "PaletteHandler.h"

QString InfoEngine::s_standardContent = "<html>"
                                        "    <head>"
                                        "        <style type=\"text/css\">body {text-align:center}</style>"
                                        "    </head>"
                                        "    <body>"
                                        "        <b>%%SUBJECT_NAME%%</b>"
                                        "    </body>"
                                        "</html>";

InfoEngine::InfoEngine( QObject* parent )
    : QObject( parent )
{
    DEBUG_BLOCK

    The::infoProxy()->subscribe( this );

    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &InfoEngine::serviceChanged );
}

InfoEngine::~InfoEngine()
{
    The::infoProxy()->unsubscribe( this );
}

QString InfoEngine::mainInfo() const
{
    QString main_info;

    if( m_storedInfo.contains("main_info") )
    {
        auto palette = The::paletteHandler()->palette();
        main_info = m_storedInfo.value("main_info").toString();
        main_info.replace("{text_color}", palette.windowText().color().name());
        main_info.replace("{content_background_color}", palette.window().color().name());
        main_info.replace("{background_color}", palette.window().color().name());
        main_info.replace("{border_color}", palette.text().color().name());
    }

    if( main_info.isEmpty() )
        main_info = s_standardContent.replace("%%SUBJECT_NAME%%", serviceName());

    return main_info;
}

void InfoEngine::infoChanged( QVariantMap infoMap )
{
    m_storedInfo = infoMap;
    Q_EMIT serviceChanged();
}

