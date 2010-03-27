/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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
 
#include "ElidingButton.h"

#include "core/support/Debug.h"

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
}

QSizePolicy ElidingButton::sizePolicy() const
{
    //This has got to be the mother of all hacks...
    //If the text is currently elided, the button should try to get more space. If not elided
    //then the button has all the space it needs and should really not try to expand beyond this.
    //Since the size hint depends on the actual text shown (which is very short when elided) we
    //cannot depend on this for making the button grow again...
    if( !m_isElided )
        return QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
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
    QPushButton::setText( elidedText );

    bool elided = ( elidedText != m_fullText );

    // If there is no tooltip set, then we set it to be the full text when elided,
    // and clear it if the button is no longer elided.
    const QString tip = toolTip();
    if( elided && tip.isEmpty() )
        setToolTip( m_fullText );
    else if( !elided && tip == m_fullText )
        setToolTip( QString() );

    if ( m_isElided )
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    else
        setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    if ( m_isElided != elided )
    {
        m_isElided = elided;
        emit( sizePolicyChanged() );
    }
}

}

#include "ElidingButton.moc"
