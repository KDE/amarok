/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2008-2009 Seb Ruiz <ruiz@kde.org>                                      *
 * Copyright (c) 2009 Roman Jarosz <kedgedev@gmail.com>                                 *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#include "Token.h"

#include <KColorScheme>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>

#include "TokenDropTarget.h"

Token * TokenFactory::createToken(const QString & text, const QString & iconName, qint64 value, QWidget * parent)
{
    return new Token( text, iconName, value, parent );
}


Token::Token( const QString &name, const QString &iconName, qint64 value, QWidget *parent )
    : QWidget( parent )
    , m_name( name )
    , m_icon( KIcon( iconName ) )
    , m_iconName( iconName )
    , m_value( value )
{
    setAttribute( Qt::WA_Hover );
    if( parent )
    {
        if ( TokenDropTarget *editWidget = qobject_cast<TokenDropTarget*>( parent ) )
            connect( this, SIGNAL(changed()), editWidget, SIGNAL(changed()) );
    }

    m_label = new QLabel( this );
    m_label->setAlignment( Qt::AlignCenter );
    m_label->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_label->setText( name );

    m_iconContainer = new QLabel( this );
    m_iconContainer->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    QPixmap pixmap = QPixmap( icon().pixmap( 16, 16 ) );
    m_iconContainer->setPixmap( pixmap );

    QHBoxLayout *hlayout = new QHBoxLayout( this );
    hlayout->setContentsMargins( 0, 0, 0, 0 );
    hlayout->addWidget( m_iconContainer );
    hlayout->addWidget( m_label );
    setLayout( hlayout );

    setContentsMargins( 4, 2, 4, 2 ); // we need extra space for the border
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}

QString
Token::name() const
{
    return m_name;
} 

qint64
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

QColor Token::textColor() const
{
    return m_label->palette().color( QPalette::Foreground );
}

void Token::setTextColor( QColor textColor )
{
    if( textColor == this->textColor() )
        return;
    QPalette myPalette( m_label->palette() );
    myPalette.setColor( QPalette::Foreground, textColor );
    m_label->setPalette( myPalette );
}

void Token::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED( pe )

    QPainter p( this );
    p.setBrush( Qt::NoBrush );
    p.setRenderHint( QPainter::Antialiasing );
    QColor c;
    if ( hasFocus() )
    {
        c = KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color();
    }
    else
    {
        c = palette().color( foregroundRole() );
        c.setAlpha( c.alpha() * 0.5 );
    }
    p.setPen( QPen( c, 2 ) );
    p.drawRoundedRect( rect().adjusted(1,1,-1,-1), 4, 4 );
    p.end();
}


