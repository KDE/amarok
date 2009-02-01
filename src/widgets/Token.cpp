/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *               2008-2009 Seb Ruiz <ruiz@kde.org>                            *
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
#include <QHBoxLayout>


Token * TokenFactory::createToken(const QString & text, const QString & iconName, int value, QWidget * parent)
{
    return new Token( text, iconName, value, parent );
}


Token::Token( const QString &name, const QString &iconName, int value, QWidget *parent )
    : QFrame( parent )
    , m_name( name )
    , m_icon( KIcon( iconName ) )
    , m_iconName( iconName )
    , m_value( value )
{
    m_label = new QLabel( this );
    m_label->setAlignment( Qt::AlignCenter );
    m_label->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_label->setText( name );

    QHBoxLayout *hlayout = new QHBoxLayout( this );
    setLayout( hlayout );
    
    m_iconContainer = new QLabel( this );
    m_iconContainer->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    QPixmap pixmap = QPixmap( icon().pixmap( 16, 16 ) );
    m_iconContainer->setPixmap( pixmap );

    setContentsMargins( 0, 0, 0, 0 );

    hlayout->setContentsMargins( 0, 0, 0, 0 );
    hlayout->addWidget( m_iconContainer );
    hlayout->addWidget( m_label );
    
    const uint borderColor = static_cast<uint>( KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color().rgb() );
    const QString hexBorder = QString::number( borderColor, 16 ).remove( 0, 2 ); // remove the Alpha channel (first two hex bits)
    
    setStyleSheet( "Token {\
        color: palette( Base );\
        border: 2px solid #" + hexBorder + ";\
        border-radius: 4px;\
        padding: 2px;\
    }" );

    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, m_label->text() );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_label->setSizePolicy( sizePolicy );
    
    m_iconContainer->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
}

QString
Token::name() const
{
    return m_name;
} 

int
Token::value() const
{
    return m_value;
}

KIcon
Token::icon() const
{
    return m_icon;
}

QString Token::iconName() const
{
    return m_iconName;
}


