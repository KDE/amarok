/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2007 Mark Kretschmann <markey@web.de>                   *
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
 
#include "SvgTinter.h"

#include "app.h"
#include "debug.h"

SvgTinter * SvgTinter::m_instance = 0;

SvgTinter * SvgTinter::instance()
{
    
    if ( m_instance == 0 )
        m_instance = new SvgTinter();
    
    return m_instance;
}

SvgTinter::SvgTinter()
{
    //for testing
    QList<QString> baseColors;
    baseColors << "#666765";
    baseColors << "#878782";
    baseColors << "#e8e8e8";
    baseColors << "#cdcec9";
    baseColors << "#444444";

    //QColor systemColor = QPalette::Window;
    
    init( App::instance()->palette().window().color(), baseColors, 20 );

    debug() << "QPalette::Window: " << App::instance()->palette().highlight().color().name();
    
}


SvgTinter::~SvgTinter()
{
}


QString SvgTinter::tint(QString filename)
{
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QString svg_source( file.readAll() );


    foreach ( QString colorName, m_tintMap.keys() ) {
        //debug() << "replace " <<  colorName << " with " << m_tintMap.value( colorName );
        svg_source.replace( colorName, m_tintMap.value( colorName ) );
    }

    return svg_source;

}


void SvgTinter::init(QColor systemColor, QList< QString > baseColorNames, int tintPercentage)
{

    foreach( QString baseColorName, baseColorNames ) {

        QColor baseColor( baseColorName );
        QColor tintedColor = blendColors( systemColor, baseColor, tintPercentage );
        m_tintMap.insert( baseColorName, tintedColor.name() );

    }

}


QColor SvgTinter::blendColors( const QColor& color1, const QColor& color2, int percent )
{
    const float factor1 = ( 100 - ( float ) percent ) / 100;
    const float factor2 = ( float ) percent / 100;

    const int r = static_cast<int>( color1.red() * factor1 + color2.red() * factor2 );
    const int g = static_cast<int>( color1.green() * factor1 + color2.green() * factor2 );
    const int b = static_cast<int>( color1.blue() * factor1 + color2.blue() * factor2 );

    QColor result;
    result.setRgb( r, g, b );

    return result;
}

namespace The {
    AMAROK_EXPORT SvgTinter* svgTinter() { return SvgTinter::instance(); }
}

