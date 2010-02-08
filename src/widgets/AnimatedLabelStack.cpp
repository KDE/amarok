/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
* Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "AnimatedLabelStack.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

static const int frameTime = 50;
static const int normalDisplayTime = 7000;

AnimatedLabelStack::AnimatedLabelStack( const QStringList &data, QWidget *p, Qt::WindowFlags f ): QWidget(p, f)
    , m_align(Qt::AlignCenter)
    , m_animTimer(0)
    , m_sleepTimer(0)
    , m_time(0)
    , m_fadeTime(300)
    , m_displayTime(normalDisplayTime)
    , m_index(0)
    , m_visibleIndex(0)
    , m_opacity(255)
    , m_targetOpacity(255)
    , m_animated(true)
    , m_pulsating(false)
    , m_pulseRequested(false)
    , m_explicit(false)
    , m_isClick(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setData( data );
}

void
AnimatedLabelStack::activateOnEnter()
{
    if ( m_data.isEmpty() || !underMouse() || m_pulsating || m_explicit )
        return;
    if ( m_animated )
    {
        m_pulseRequested = true;
        if ( m_time > m_fadeTime && m_time < (m_displayTime - m_fadeTime) )
            m_time = m_displayTime - m_fadeTime;
        wakeUp();
    }
    else
        setPulsating( true );
}

void
AnimatedLabelStack::ensureAnimationStatus()
{
    if ( m_data.count() > 1 && ( m_animated || m_pulsating ) )
    {
        wakeUp();
    }
    else
    {
        if ( m_animTimer )
        {
            killTimer( m_animTimer );
            m_animTimer = 0;
        }
        if ( m_sleepTimer )
        {
            killTimer( m_sleepTimer );
            m_sleepTimer = 0;
        }
        m_opacity = m_targetOpacity;
        update();
    }
}

void
AnimatedLabelStack::enterEvent( QEvent * )
{
    // wait a short time, then pulse through entries
    m_explicit = false;
    QTimer::singleShot(300, this, SLOT( activateOnEnter() ) );
}

void
AnimatedLabelStack::hideEvent( QHideEvent *e )
{
    QWidget::hideEvent( e );
    if ( m_animTimer )
    {
        killTimer( m_animTimer );
        m_animTimer = 0;
    }
    if ( m_sleepTimer )
    {
        killTimer( m_sleepTimer );
        m_sleepTimer = 0;
    }
    m_opacity = m_targetOpacity;
}

void
AnimatedLabelStack::leaveEvent( QEvent * )
{
    m_explicit = false;
    m_pulseRequested = false;
}

void
AnimatedLabelStack::mousePressEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton || m_data.isEmpty() )
        return;

    m_isClick = true;
    me->accept();
}

void
AnimatedLabelStack::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton || m_data.isEmpty() )
        return;

    me->accept();
    if ( m_isClick && underMouse() )
    {
        m_isClick = false;
        if ( !m_data.isEmpty() )
            emit clicked ( m_data.at( m_visibleIndex ) );
    }
}

void
AnimatedLabelStack::paintEvent( QPaintEvent * pe )
{
    if ( m_data.isEmpty() )
        return;
    
    QPainter p(this);
    p.setClipRegion( pe->region() );

    QColor c( palette().color( foregroundRole() ) );
    c.setAlpha( m_targetOpacity );

    if ( m_animTimer ) // currently animated
    {
        if ( m_opacity != m_targetOpacity ) // we're in transition period
        {
            if ( !m_pulsating )
            {
                c.setAlpha( qAbs(m_targetOpacity - m_opacity) );
                p.setPen( c );
                int index = m_visibleIndex - 1;
                if (index < 0)
                    index = m_data.count() - 1;

                p.drawText( rect(), m_align | Qt::TextSingleLine, elidedText( m_data.at( index ) ) );
            }
            
            c.setAlpha( m_opacity );
        }
    }
    
    p.setPen( c );
    p.drawText( rect(), m_align | Qt::TextSingleLine, elidedText( m_data.at( m_visibleIndex ) ) );
    p.end();
}

void
AnimatedLabelStack::showEvent( QShowEvent *e )
{
    ensureAnimationStatus();
    QWidget::showEvent( e );
}


QString
AnimatedLabelStack::elidedText( const QString& text ) const
{
    const QFontMetrics fontMetrics( font() );

    QString newText = fontMetrics.elidedText( text, Qt::ElideRight, width() / 1.7 );

    // Insert a whitespace between text and "..." (looks nicer)
    if( newText != text )
        newText.insert( newText.length() -1, ' ' );


    return newText;
}

void
AnimatedLabelStack::pulse( int /*cycles*/, int /*minimum*/ )
{
    //TODO: handle parameters...
    activateOnEnter();
}

void
AnimatedLabelStack::setAlign( Qt::Alignment align )
{
    m_align = Qt::AlignVCenter;
    if ( align & Qt::AlignLeft )
        m_align |= Qt::AlignLeft;
    else if ( align & Qt::AlignRight )
        m_align |= Qt::AlignRight;
    else
        m_align = Qt::AlignCenter;
}


void
AnimatedLabelStack::setAnimated( bool on )
{
    m_animated = on;
    ensureAnimationStatus();
}

void
AnimatedLabelStack::setBold( bool bold )
{
    QFont fnt = font();
    fnt.setBold(bold);
    setFont(fnt);
    setMinimumHeight( QFontMetrics(fnt).height() + 4 );
}

void
AnimatedLabelStack::setData( const QStringList &data  )
{
    if ( data == m_data )
        return;
    m_data = data;
    m_time = 0;
    m_index = 0;
    m_visibleIndex = 0;
    ensureAnimationStatus();
    update();
}

void
AnimatedLabelStack::setPulsating( bool on )
{
    if ( m_pulseRequested == on && m_pulsating == on )
        return;
    m_pulseRequested = on;
    m_pulsating = on;
    if ( m_pulsating )
    {
        m_displayTime = 1200;
        m_fadeTime = 300;
        if ( m_time > m_fadeTime && m_time <  m_displayTime - m_fadeTime )
            m_time = m_displayTime - m_fadeTime + 1; // for instant reaction
    }
    else
    {
        m_displayTime = normalDisplayTime;
        m_fadeTime = 300;
        if ( !m_animated )
            m_time = m_fadeTime + 1;
    }
    ensureAnimationStatus();
    emit pulsing( on );
}

void
AnimatedLabelStack::sleep( int ms )
{
    if ( m_animTimer )
    {
        killTimer( m_animTimer );
        m_animTimer = 0;
    }
    if ( !m_sleepTimer )
        m_sleepTimer = startTimer( ms );
}

void
AnimatedLabelStack::wakeUp()
{
    if ( m_sleepTimer )
    {
        killTimer( m_sleepTimer );
        m_sleepTimer = 0;
    }
    if ( !m_animTimer )
        m_animTimer = startTimer( frameTime );
}

void
AnimatedLabelStack::timerEvent( QTimerEvent * te )
{

    if ( !isVisible() )
        return;
    if ( te->timerId() == m_sleepTimer )
        wakeUp();
    else if ( te->timerId() != m_animTimer )
        return;

    if ( m_explicit )
        return; // the user explicitly altered content by wheeling, don't take it away

    if ( m_time < m_fadeTime || m_time > (m_displayTime - m_fadeTime) )
        update();

    m_time += frameTime;
    if ( m_time > m_displayTime )
    {
        m_time = 0;
        if ( m_pulsating && !m_pulseRequested )
            m_visibleIndex = m_index;
        else
        {
            ++m_visibleIndex;
            if ( m_visibleIndex >= m_data.count() )
                m_visibleIndex = 0;
        }
        if ( !m_pulsating )
            m_index = m_visibleIndex;
    }

    if ( m_time < m_fadeTime ) // fade in
    {
        if ( m_pulseRequested && !m_pulsating )
            setPulsating( true );
        m_opacity = m_targetOpacity*m_time/m_fadeTime;
        wakeUp();
    }
    else if ( m_pulsating && m_time > (m_displayTime - m_fadeTime) ) // fade out
    {
        m_opacity = m_targetOpacity*(m_displayTime - m_time)/m_fadeTime;
        wakeUp();
    }
    else // (ensure) no fade
    {
        if ( !m_pulsating && m_time < (m_displayTime - m_fadeTime) )
        {
            m_time = m_displayTime - m_fadeTime + 1;
            sleep( m_time );
        }

        m_opacity = m_targetOpacity; // to be sure

        if ( m_pulsating && !m_pulseRequested && m_index == m_visibleIndex )
            setPulsating( false );
    }
}

void
AnimatedLabelStack::wheelEvent( QWheelEvent * we )
{
    if ( we->modifiers() & Qt::ControlModifier )
    {
        we->accept();
        if ( m_data.count() < 2 )
            return;

        setPulsating( false );

        if ( we->delta() < 0 )
        {
            ++m_visibleIndex;
            if ( m_visibleIndex >= m_data.count() )
                m_visibleIndex = 0;
        }
        else
        {
            --m_visibleIndex;
            if ( m_visibleIndex < 0 )
                m_visibleIndex = m_data.count() - 1;
        }
        m_index = m_visibleIndex;
        m_time = m_fadeTime + 1;
        m_explicit = true;
        update();
    }
    else
        we->ignore();
}

#include "AnimatedLabelStack.moc"
