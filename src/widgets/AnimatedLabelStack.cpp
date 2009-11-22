
#include "AnimatedLabelStack.h"

#include <QPainter>
#include <QPaintEvent>

#include <QtDebug>

static const int frameTime = 50;

AnimatedLabelStack::AnimatedLabelStack( const QStringList &data, QWidget *p, Qt::WindowFlags f ): QWidget(p, f)
, m_align(Qt::AlignCenter)
, m_time(0)
, m_index(0)
, m_animTimer(0)
, m_fadeTime(300)
, m_displayTime(4000)
, m_opacity(255)
, m_targetOpacity(255)
, m_animated(true)
, m_pulsating(false)
, m_pulseRequested(false)
, m_isClick(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setData( data );
}

void AnimatedLabelStack::ensureAnimationStatus()
{
    if ( m_data.count() > 1 && ( m_animated || m_pulsating ) )
    {
        if ( !m_animTimer )
            m_animTimer = startTimer( frameTime );
    }
    else if ( m_animTimer )
    {
        killTimer(m_animTimer);
        m_animTimer = 0;
        m_opacity = m_targetOpacity;
        update();
    }
}

void AnimatedLabelStack::enterEvent( QEvent * )
{
    if ( m_pulsating )
        return;
    if ( m_animated )
    {
        m_pulseRequested = true;
        if ( m_time > m_fadeTime && m_time < (m_displayTime - m_fadeTime) )
            m_time = m_displayTime - m_fadeTime;
    }
    else
        setPulsating( true );
}

void AnimatedLabelStack::leaveEvent( QEvent * )
{
    m_pulseRequested = false;
}

void AnimatedLabelStack::mousePressEvent( QMouseEvent *me )
{
    m_isClick = true;
    me->accept();
}

void AnimatedLabelStack::mouseReleaseEvent( QMouseEvent *me )
{
    me->accept();
    if ( m_isClick && underMouse() )
    {
        m_isClick = false;
        if ( !m_data.isEmpty() )
            emit clicked ( m_data.at( m_index ) );
    }
}

void AnimatedLabelStack::paintEvent( QPaintEvent * pe )
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
                int index = m_index - 1;
                if (index < 0)
                    index = m_data.count() - 1;
                p.drawText( rect(), m_align | Qt::TextSingleLine, m_data.at( index ) );
            }
            
            c.setAlpha( m_opacity );
        }
    }
    
    p.setPen( c );
    p.drawText( rect(), m_align | Qt::TextSingleLine, m_data.at( m_index ) );
    p.end();
}

void AnimatedLabelStack::pulse( int cycles, int minimum )
{
    enterEvent(0);
}

void AnimatedLabelStack::setAlign( Qt::Alignment align )
{
    m_align = Qt::AlignVCenter;
    if ( align & Qt::AlignLeft )
        m_align |= Qt::AlignLeft;
    else if ( align & Qt::AlignRight )
        m_align |= Qt::AlignRight;
    else
        m_align = Qt::AlignCenter;
}


void AnimatedLabelStack::setAnimated( bool on )
{
    m_animated = on;
    ensureAnimationStatus();
}

void AnimatedLabelStack::setBold( bool bold )
{
    QFont fnt = font();
    fnt.setBold(bold);
    setFont(fnt);
    setMinimumHeight( QFontMetrics(fnt).height() + 4 );
}

void AnimatedLabelStack::setData( const QStringList &data  )
{
    if ( data == m_data )
        return;
    m_data = data;
    m_time = 0;
    m_index = 0;
    ensureAnimationStatus();
    update();
}

void AnimatedLabelStack::setPulsating( bool on )
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
        m_displayTime = 4000;
        m_fadeTime = 300;
        if ( !m_animated )
            m_time = m_fadeTime + 1;
    }
    ensureAnimationStatus();
    emit pulsing( on );
}

void AnimatedLabelStack::timerEvent( QTimerEvent * te )
{
    if ( !isVisible() || te->timerId() != m_animTimer )
        return;
    
    if ( m_time < m_fadeTime || m_time > (m_displayTime - m_fadeTime) )
        update();
    m_time += frameTime;
    if ( m_time > m_displayTime )
    {
        m_time = 0;
        ++m_index;
        if ( m_index >= m_data.count() )
            m_index = 0;
    }

    if ( m_time < m_fadeTime ) // fade in
    {
        if ( m_pulseRequested && !m_pulsating )
            setPulsating( true );
        m_opacity = m_targetOpacity*m_time/m_fadeTime;
    }
    else if ( m_pulsating && m_time > (m_displayTime - m_fadeTime) ) // fade out
    {
        m_opacity = m_targetOpacity*(m_displayTime - m_time)/m_fadeTime;
    }
    else // (ensure) no fade
    {
        if ( m_pulsating && !m_pulseRequested )
            setPulsating( false );
        m_opacity = m_targetOpacity; // to be sure
    }
}

void AnimatedLabelStack::wheelEvent( QWheelEvent * we )
{
    if ( m_data.count() < 2 )
        return;

    setPulsating( false );
    
    if ( we->delta() < 0 )
    {
        ++m_index;
        if ( m_index >= m_data.count() )
            m_index = 0;
    }
    else
    {
        --m_index;
        if ( m_index < 0 )
            m_index = m_data.count() - 1;
    }
    update();
}
