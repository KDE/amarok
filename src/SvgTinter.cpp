/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "core/support/Debug.h"

#include <QBuffer>
#include <QFile>

#include <KCompressionDevice>

SvgTinter * SvgTinter::s_instance = nullptr;

SvgTinter::SvgTinter()
    : m_firstRun( true )
{
    init();
    m_firstRun = false;
}


SvgTinter::~SvgTinter()
{}

QByteArray
SvgTinter::tint( const QString &filename )
{
    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        error() << "Unable to open file: " << filename;
        return QByteArray();
    }

    QByteArray svg_source( file.readAll() );

    // Copied from KSvgrenderer.cpp as we don't load it directly.
    if (!svg_source.startsWith("<?xml"))
    {
        QBuffer buf( &svg_source );
        QIODevice *flt = new KCompressionDevice( &buf, false, KCompressionDevice::GZip );
        if (!flt)
            return QByteArray();
        if (!flt->open(QIODevice::ReadOnly))
        {
            delete flt;
            return QByteArray();
        }
        svg_source = flt->readAll();
        delete flt;
    }

    // QString svg_string( svg_source );
    QHashIterator<QByteArray, QString> tintIter( m_tintMap );
    while( tintIter.hasNext() )
    {
        tintIter.next();
        svg_source.replace( tintIter.key(), tintIter.value().toLocal8Bit() );
    }
    return svg_source;
}

void
SvgTinter::init()
{
    if ( m_lastPalette != pApp->palette() || m_firstRun ) {
        m_tintMap.insert( "#666765", pApp->palette().window().color().name() );
        //insert a color for bright ( highlight color )
        m_tintMap.insert( "#66ffff", pApp->palette().highlight().color().name() );
        //a slightly lighter than window color:
        m_tintMap.insert( "#e8e8e8", blendColors( pApp->palette().window().color(), "#ffffff", 90 ).name() );
        //a slightly darker than window color:
        m_tintMap.insert( "#565755", blendColors( pApp->palette().window().color(), "#000000", 90 ).name() );

        //list background:
    #ifdef Q_OS_APPLE
        m_tintMap.insert( "#f0f0f0", blendColors( pApp->palette().window().color(), "#000000", 90 ).name() );
        m_tintMap.insert( "#ffffff", blendColors( pApp->palette().window().color(), "#000000", 98 ).name() );
    #else
       m_tintMap.insert( "#f0f0f0", pApp->palette().base().color().name() );
    #endif

        //alternate list background:
        m_tintMap.insert( "#e0e0e0", pApp->palette().alternateBase().color().name() );

        //highlight/window mix:
        m_tintMap.insert( "#123456", blendColors( pApp->palette().window().color(), pApp->palette().highlight().color().name(), 80 ).name() );

        //text color, useful for adding contrast
        m_tintMap.insert( "#010101", pApp->palette().text().color().name() );

        m_lastPalette = pApp->palette();
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
        if ( SvgTinter::s_instance == nullptr )
            SvgTinter::s_instance = new SvgTinter();

        return SvgTinter::s_instance;
    }
}



