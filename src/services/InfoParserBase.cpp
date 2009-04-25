/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "InfoParserBase.h"

#include "App.h"
#include "Debug.h"
#include "PaletteHandler.h"

#include <KStandardDirs>

#include <QFile>
#include <QPalette>

QString InfoParserBase::s_loadingBaseHtml = QString();

InfoParserBase::InfoParserBase()
  : QObject()
{

}

void InfoParserBase::showLoading( const QString &message )
{

    DEBUG_BLOCK
    if ( s_loadingBaseHtml.isEmpty() ) {

        const KUrl url( KStandardDirs::locate( "data", "amarok/data/" ) );
        QString htmlFile = url.path() + "InfoParserLoading.html";

        if ( !QFile::exists( htmlFile ) )
        {
            debug() << "file " << htmlFile << "does not exist";
            return;
        }

        QFile file( htmlFile );
        if( !file.open( QIODevice::ReadOnly ) )
        {
            debug() << "error reading file " << htmlFile;
            return;
        }

        QString html;
        while (!file.atEnd()) {
            html += file.readLine();
        }

        s_loadingBaseHtml = html;
    }

    QString currentHtml = s_loadingBaseHtml;

    const KUrl url( KStandardDirs::locate( "data", "amarok/images/" ) );
    currentHtml = currentHtml.replace( "%%IMAGEPATH%%", url.url() );
    currentHtml = currentHtml.replace( "%%TEXT%%", message );

    QColor highlight( App::instance()->palette().highlight().color() );
    highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
    currentHtml = currentHtml.replace( "{text_color}", App::instance()->palette().brush( QPalette::Text ).color().name() );
    currentHtml = currentHtml.replace( "{content_background_color}", highlight.name() );
    currentHtml = currentHtml.replace( "{background_color}", PaletteHandler::highlightColor().lighter( 150 ).name());
    currentHtml = currentHtml.replace( "{border_color}", PaletteHandler::highlightColor().lighter( 150 ).name() );
    debug() << "showing html: " << currentHtml;
    emit ( info( currentHtml ) );

}

#include "InfoParserBase.moc"

