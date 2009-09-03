/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2007 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "SvgTinter.h"

#include "App.h"
#include "Debug.h"

#include <QBuffer>
#include <KFilterDev>

SvgTinter * SvgTinter::s_instance = 0;

SvgTinter::SvgTinter()
    : m_firstRun( true )
{
    init();
    m_firstRun = false;
}


SvgTinter::~SvgTinter()
{}

QString
SvgTinter::tint(QString filename)
{
    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        error() << "Unable to open file: " << filename;
        return QString();
    }

    QByteArray svg_source( file.readAll() );

    // Copied from KSvgrenderer.cpp as we don't load it directly.
    if (!svg_source.startsWith("<?xml"))
    {
        QBuffer buf( &svg_source );
        QIODevice *flt = KFilterDev::device( &buf, QString::fromLatin1("application/x-gzip"), false );
        if (!flt)
            return QString();
        if (!flt->open(QIODevice::ReadOnly))
        {
            delete flt;
            return QString();
        }
        svg_source = flt->readAll();
        delete flt;
    }

    QString svg_string( svg_source );
    foreach ( const QString &colorName, m_tintMap.keys() ) {
        //debug() << "replace " <<  colorName << " with " << m_tintMap.value( colorName );
        svg_string.replace( colorName, m_tintMap.value( colorName ) );
    }

    return svg_string;
}

void
SvgTinter::init()
{
    if ( m_lastPalette != App::instance()->palette() || m_firstRun ) {
        m_tintMap.insert( "#666765", App::instance()->palette().window().color().name() );
        //insert a color for bright ( highlight color )
        m_tintMap.insert( "#66ffff", App::instance()->palette().highlight().color().name() );
        //a slightly lighter than window color:
        m_tintMap.insert( "#e8e8e8", blendColors( App::instance()->palette().window().color(), "#ffffff", 90 ).name() );
        //a slightly darker than window color:
        m_tintMap.insert( "#565755", blendColors( App::instance()->palette().window().color(), "#000000", 90 ).name() );

        //list background:
    #ifdef Q_WS_MAC 
        m_tintMap.insert( "#f0f0f0", blendColors( App::instance()->palette().window().color(), "#000000", 90 ).name() );
        m_tintMap.insert( "#ffffff", blendColors( App::instance()->palette().window().color(), "#000000", 98 ).name() );
    #else
       m_tintMap.insert( "#f0f0f0", App::instance()->palette().base().color().name() );
    #endif

        //alternate list background:
        m_tintMap.insert( "#e0e0e0", App::instance()->palette().alternateBase().color().name() );

        //highlight/window mix:
        m_tintMap.insert( "#123456", blendColors( App::instance()->palette().window().color(), App::instance()->palette().highlight().color().name(), 80 ).name() );

        //text color, useful for adding contrast
        m_tintMap.insert( "#010101", App::instance()->palette().text().color().name() );

        m_lastPalette = App::instance()->palette();
    }
}

QColor
SvgTinter::blendColors( const QColor& color1, const QColor& color2, int percent )
{
    const float factor1 = ( float ) percent / 100;
    const float factor2 = ( 100 - ( float ) percent ) / 100;

    const int r = static_cast<int>( color1.red() * factor1 + color2.red() * factor2 );
    const int g = static_cast<int>( color1.green() * factor1 + color2.green() * factor2 );
    const int b = static_cast<int>( color1.blue() * factor1 + color2.blue() * factor2 );

    QColor result;
    result.setRgb( r, g, b );

    return result;
}

namespace The {
    SvgTinter*
    svgTinter()
    {
        if ( SvgTinter::s_instance == 0 )
            SvgTinter::s_instance = new SvgTinter();

        return SvgTinter::s_instance;
    }
}



