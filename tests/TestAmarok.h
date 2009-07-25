/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
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

#ifndef TESTAMAROK_H
#define TESTAMAROK_H

#include <QtTest>

class TestAmarok : public QObject
{
Q_OBJECT

public:
    TestAmarok( QStringList testArgumentList );

private slots:
    void testComputeScore();
    /*QString testSaveLocation();
    KUrl::List testRecursiveUrlExpand( const KUrl &url ); //defined in PlaylistHandler.cpp
    KUrl::List testRecursiveUrlExpand( const KUrl::List &urls ); //defined in PlaylistHandler.cpp
    QString testVerboseTimeSince( const QDateTime &datetime ); // defined in App.cpp
    QString testVerboseTimeSince( uint time_t ); // defined in App.cpp
    QString testConciseTimeSince( uint time_t ); // defined in App.cpp
    void testAlbumArtistTrackFromUrl( QString url, QString &artist, QString &album, QString &detail ); // TODO: needs testdata
    */
    void testExtension();
    //KUrl testMostLocalURL( const KUrl &url );
 /*   QStringList splitPath( QString path );
    QString cleanPath( const QString &path );
    QString asciiPath( const QString &path );
    QString vfatPath( const QString &path );
    QString decapitateString( const QString &input, const QString &ref );
    QString escapeHTMLAttr( const QString &s );
    QString unescapeHTMLAttr( const QString &s );
    QString proxyForUrl( const QString& url ); // how to test?
    QString proxyForProtocol( const QString& protocol ); // how to test?     */
    void testManipulateThe();
};

#endif // TESTAMAROK_H
