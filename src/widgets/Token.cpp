/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#include "Token.h"
#include "Debug.h"

Token::Token( const QString &string, QWidget *parent )
    : QLabel( parent )
{
    m_myCount = qobject_cast< FilenameLayoutWidget * >( parent )->getTokenCount();

    setText( string );
    setTokenString( string );
    setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    setStyleSheet( "Token {\
        color: palette( Base );\
        background-color: qlineargradient( x1: 0,\
                                           y1: 0,\
                                           x2: 1,\
                                           y2: 1,\
                                           stop: 0 white,\
                                           stop: 0.4 gray,\
                                           stop: 1 blue );\
    }" );

    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, text() );
    setMinimumSize( size + QSize( 4, 0 ) );
}

//Access for m_tokenString, private.
void
Token::setTokenString( const QString &string )
{
    m_tokenString = string;
}

//Access for m_tokenString.
QString
Token::getTokenString()
{
    return m_tokenString;
}


