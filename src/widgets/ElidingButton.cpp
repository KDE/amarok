/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2009  Seb Ruiz <ruiz@kde.org>                           *
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
 
#include "ElidingButton.h"

#include "Debug.h"

#include <QFontMetrics>

namespace Amarok {

ElidingButton::ElidingButton( QWidget *parent )
    : QPushButton( parent )
{
    init();
}

ElidingButton::ElidingButton( const QString & text, QWidget * parent )
    : QPushButton( text, parent )
    , m_fullText( text )
{
    init();
}

ElidingButton::ElidingButton( const QIcon & icon, const QString & text, QWidget * parent )
    : QPushButton( icon, text, parent )
    , m_fullText( text )
{
    init();
}

ElidingButton::~ElidingButton()
{
}

void ElidingButton::init()
{
    m_isElided = false;
    setMinimumWidth( iconSize().width() );
    setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
}

bool ElidingButton::isElided() const
{
    return m_isElided;
}

void ElidingButton::resizeEvent( QResizeEvent * event )
{
    elideText( event->size() );
    QPushButton::resizeEvent( event );
}

void ElidingButton::setText( const QString &text )
{
    m_fullText = text;
    elideText( size() );
    // elideText will call QPushButton::setText()
}

void ElidingButton::elideText( const QSize &widgetSize )
{
    const int width = widgetSize.width();
    const int iconWidth = iconSize().width();

    int left, top, right, bottom;
    getContentsMargins( &left, &top, &right, &bottom );
    int padding = left + right + 4;

    int textWidth = width - ( iconWidth + padding );

    QFontMetrics fm( font() );
    QString elidedText = fm.elidedText( m_fullText, Qt::ElideRight, textWidth );

    m_isElided = elidedText != m_fullText;

    QPushButton::setText( elidedText );
}

}

#include "ElidingButton.moc"
