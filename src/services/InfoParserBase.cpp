/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "InfoParserBase.h"

#include "core/support/Debug.h"

#include <QStandardPaths>
#include <QUrl>

#include <QFile>
#include <QPalette>

QString InfoParserBase::s_loadingBaseHtml;

InfoParserBase::InfoParserBase()
  : QObject()
{}

void InfoParserBase::showLoading( const QString &message )
{
    DEBUG_BLOCK

    if( s_loadingBaseHtml.isEmpty() )
    {
        const QUrl url( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/data/") ) );
        QString htmlFile = url.path() + QStringLiteral("InfoParserLoading.html");

        if( !QFile::exists( htmlFile ) )
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

        s_loadingBaseHtml = file.readAll();
    }

    QString currentHtml = s_loadingBaseHtml;

    const QUrl url( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/") ) );
    currentHtml = currentHtml.replace( QLatin1String("%%IMAGEPATH%%"), url.url() );
    currentHtml = currentHtml.replace( QLatin1String("%%TEXT%%"), message );

    // debug() << "showing html: " << currentHtml;
    Q_EMIT ( info( currentHtml ) );
}


