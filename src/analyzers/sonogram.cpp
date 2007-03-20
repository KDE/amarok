//
//
// C++ Implementation: Sonogram
//
// Description:
//
//
// Author: Melchior FRANZ <mfranz@kde.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <QPainter>
//Added by qt3to4:
#include <QResizeEvent>
#include <QPaintEvent>
#include "debug.h"
#include "sonogram.h"

Sonogram::Sonogram(QWidget *parent) :
    Analyzer::Base2D(parent, 16, 9)
{
}


Sonogram::~Sonogram()
{
}


void Sonogram::init()
{
}


void Sonogram::resizeEvent(QResizeEvent *e)
{
    DEBUG_BLOCK

//only for gcc < 4.0
#if !( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 0 ) )
    resizeForBands(height() < 128 ? 128 : height());
#endif

        Analyzer::Base2D::resizeEvent( e );
//     p.drawPixmap( 0, 0, background() )
//     bitBlt(this, 0, 0, background());
}


void Sonogram::analyze(const Scope &s)
{
//     Analyzer::interpolate( s, m_scope );
    update();
}

void
Sonogram::paintEvent( QPaintEvent * )
{
    int x = width() - 1;
    QColor c;
    QPainter p( this );

//     bitBlt(canvas(), 0, 0, canvas(), 1, 0, x, height());
//     p.drawPixmap( 0, 0, this, 1, 0, x, height() );
    const Scope &s = m_scope;
    Scope::const_iterator it = s.begin(), end = s.end();
    for (int y = height() - 1; y;) {
        if (it >= end || *it < .005)
            c = backgroundColor();
        else if (*it < .05)
            c.setHsv(95, 255, 255 - int(*it * 4000.0));
        else if (*it < 1.0)
            c.setHsv(95 - int(*it * 90.0), 255, 255);
        else
            c = Qt::red;

        p.setPen(c);
        p.drawPoint(x, y--);

        if (it < end)
            ++it;
    }
}


void Sonogram::transform(Scope &scope)
{
    float *front = static_cast<float*>(&scope.front());
    m_fht->power2(front);
    m_fht->scale(front, 1.0 / 256);
    scope.resize( m_fht->size() / 2 );
}


void Sonogram::demo()
{
    analyze(Scope(m_fht->size(), 0));
}

