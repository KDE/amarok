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
#include "FilenameLayoutWidget.h"
#include "Debug.h"

#include <KColorScheme>

Token::Token( const QString &string, QWidget *parent )
    : QFrame( parent )
{
    m_myCount = qobject_cast< FilenameLayoutWidget * >( parent )->getTokenCount();
    //m_icon = new QPixmap( "placeholder.png"); //TODO: get icons from oxygen guys and handle loading
    m_label = new QLabel( this );
    hlayout = new QHBoxLayout( this );
    setLayout( hlayout );
    QLabel *iconContainer = new QLabel( this );
    //m_icon->something to get a pixmap here
    //iconContainer->setPixmap( *m_icon );
    setContentsMargins( 0, 0, 0, 0 );
    hlayout->setContentsMargins( 0, 0, 0, 0 );
    hlayout->addWidget( iconContainer );
    hlayout->addWidget( m_label );
    setString( string );
    m_label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    unsigned int borderColor = static_cast<unsigned int>( KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color().rgb() );
    setStyleSheet( "Token {\
        color: palette( Base );\
        border: 2px solid #" + QString::number( borderColor, 16 ).remove( 0, 2 ) + ";\
        border-radius: 4px;\
        padding: 2px;\
    }" );       //I use QString::remove(int start, int n) to remove the A channel from ARGB - first two characters

    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, m_label->text() );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_label->setSizePolicy( sizePolicy );
    //debug stuff, remove when done:
    m_label->setAutoFillBackground(true);
    m_label->setPalette(QPalette(QColor(0,0,0),QColor(255,0,0)));
    
    iconContainer->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
}

//Access for m_tokenString, private.
void
Token::setString( const QString &string )
{
    m_label->setText( string );
    m_tokenString = string;
    //TODO: code to set icon
}

//Access for m_tokenString.
QString
Token::getString()
{
    return m_tokenString;
}

void //does this do any good?
Token::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event );
    hlayout->update(); //does this do any good?
}
