/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
    , m_isElided( false )
{
    setMinimumWidth( iconSize().width() );
}

ElidingButton::ElidingButton( const QString & text, QWidget * parent )
    : QPushButton( text, parent )
    , m_fullText( text )
    , m_isElided( false )
{
    setMinimumWidth( iconSize().width() );
}

ElidingButton::ElidingButton( const QIcon & icon, const QString & text, QWidget * parent )
    : QPushButton( icon, text, parent )
    , m_fullText( text )
    , m_isElided( false )
{
    setMinimumWidth( iconSize().width() );
}

ElidingButton::~ElidingButton()
{
}

QSizePolicy ElidingButton::sizePolicy() const
{
    DEBUG_BLOCK

    //This has got to be the mother of all hacks... 
    //If the text is currently elided, the button should try to get more space. If not elided
    //then the button has all the space it needs and should really not try to expand beyond this.
    //Since the size hint depends on the actual text shown (which is very short when elided) we
    //cannot depend on this for making the button grow again...
    if( !m_isElided )
        return QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}

void ElidingButton::resizeEvent( QResizeEvent * event )
{
    DEBUG_BLOCK
    debug() << "old size: " << event->oldSize();
    debug() << "suggested new size: " << event->size();

    //elide text...

    int width = event->size().width();
    int iconWidth = iconSize().width();

    int left, top, right, bottom;
    getContentsMargins ( &left, &top, &right, &bottom );
    int padding = left + right + 4;

    int textWidth = width - ( iconWidth + padding );

    QFontMetrics fm( font() );
    QString elidedText = fm.elidedText( m_fullText, Qt::ElideRight, textWidth );
    setText( elidedText );

    bool elided = ( elidedText != m_fullText );
    
    if ( m_isElided )
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    else
        setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    if ( m_isElided != elided )
    {
        m_isElided = elided;
        emit( sizePolicyChanged() );
    }

    debug() << "m_isElided: " << m_isElided;

    QPushButton::resizeEvent( event );
}

void ElidingButton::setFixedHeight( int h )
{
    Q_UNUSED( h );
}

void ElidingButton::setFixedSize( const QSize &s )
{
    Q_UNUSED( s );
}

void ElidingButton::setFixedSize( int w, int h )
{
    Q_UNUSED( w );
    Q_UNUSED( h );
}

void ElidingButton::setFixedWidth( int w )
{
    Q_UNUSED( w );
}

}

#include "ElidingButton.moc"
