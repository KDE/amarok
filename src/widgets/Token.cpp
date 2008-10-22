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

// Token::Token( QWidget *parent )
//     :QFrame( parent )
// {
// 
//     
// }

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
    hlayout->addWidget( iconContainer );
    hlayout->addWidget( m_label );
    //Token( parent );
    setString( string );
    m_label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    setStyleSheet( "Token {\
        color: palette( Base );\
        border: 2px solid blue;\
        border-radius: 4px;\
        padding: 2px;\
    }" );
/*
        background-color: qlineargradient( x1: 0,\
                                           y1: 0,\
                                           x2: 1,\
                                           y2: 1,\
                                           stop: 0 white,\
                                           stop: 0.4 gray,\
                                           stop: 1 blue );\


*/
    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, m_label->text() );
    m_label->setMinimumSize( size + QSize( 4, 0 ) );
    m_label->setUpdatesEnabled( true ); //does this do any good?
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_label->setSizePolicy( sizePolicy );
    iconContainer->setSizePolicy( sizePolicy );
    iconContainer->setUpdatesEnabled( true ); //does this do any good?
    setUpdatesEnabled( true ); //does this do any good?
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

