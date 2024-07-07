/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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

#include "PlayPauseButton.h"

#include "SvgHandler.h"

#include <KLocalizedString>

#include <QMouseEvent>
#include <QPainter>


PlayPauseButton::PlayPauseButton( QWidget *parent ) : IconButton( parent )
    , m_isPlaying( false )
{
    connect (this, &PlayPauseButton::clicked, this, &PlayPauseButton::toggle );
    setToolTip( i18n( "Play" ) );
}


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void PlayPauseButton::enterEvent ( QEvent * )
#else
void PlayPauseButton::enterEvent ( QEnterEvent * )
#endif
{
    setIcon( m_isPlaying ? m_icon.pause[1] : m_icon.play[1], 3 );
}

void PlayPauseButton::leaveEvent( QEvent * )
{
    setIcon( m_isPlaying ? m_icon.pause[0] : m_icon.play[0], 6 );
}

void PlayPauseButton::mousePressEvent( QMouseEvent *me )
{
    setIcon( m_isPlaying ? m_icon.pause[0] : m_icon.play[0] );
    IconButton::mousePressEvent( me );
}

void PlayPauseButton::toggle()
{
    Q_EMIT toggled( !m_isPlaying );
}

void PlayPauseButton::reloadContent( const QSize &sz )
{
    const int width  = sz.width()*2; //double size svg render to have better looking high-dpi toolbar
    const int height = sz.height()*2;
    //NOTICE this is a bit cumbersome, as Qt renders faster to images than to pixmaps
    // However we need the Image and generate the pixmap ourself - maybe extend the SvgHandler API
    m_icon.play[0] = The::svgHandler()->renderSvg( QStringLiteral("PLAYpause"), width, height, QStringLiteral("PLAYpause"), true ).toImage();
    m_icon.play[1] = The::svgHandler()->renderSvg( QStringLiteral("PLAYpause_active"), width, height, QStringLiteral("PLAYpause_active"), true ).toImage();
    m_icon.pause[0] = The::svgHandler()->renderSvg( QStringLiteral("playPAUSE"), width, height, QStringLiteral("playPAUSE"), true ).toImage();
    m_icon.pause[1] = The::svgHandler()->renderSvg( QStringLiteral("playPAUSE_active"), width, height, QStringLiteral("playPAUSE_active"), true ).toImage();
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 2; ++i )
        {
            m_icon.play[i] = m_icon.play[i].mirrored( true, false );
            m_icon.pause[i] = m_icon.pause[i].mirrored( true, false );
        }
    }
    setIcon( m_isPlaying ? m_icon.pause[underMouse()] : m_icon.play[underMouse()] );
}

void PlayPauseButton::setPlaying( bool playing )
{
    if ( m_isPlaying == playing )
        return;

    setToolTip( playing ? i18n( "Pause" ) : i18n( "Play" ) );

    m_isPlaying = playing;
    setIcon( m_isPlaying ? m_icon.pause[underMouse()] : m_icon.play[underMouse()], 4 );
}

