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

#include <qpainter.h>
#include "sonogram.h"

using namespace Analyzer;

Sonogram::Sonogram(QWidget *parent) :
        Analyzer::Base2D(parent, 16)
{
}


Sonogram::~Sonogram()
{
}


void Sonogram::init()
{
    eraseCanvas();
}


void Sonogram::drawAnalyzer(std::vector<float> *s)
{
        int x = width() - 1;
        QColor c;
        QPainter p(canvas());

        bitBlt(canvas(), 0, 0, canvas(), 1, 0, x, height());
        if (s) {
                std::vector<float>::iterator it = s->begin();
                for (int y = height() - 1; y && it < s->end(); it++) {
                        if (*it < .005)
                                c = backgroundColor();//Qt::black;
                        else if (*it < .05)
                                c.setHsv(95, 255, 255 - int(*it * 4000.0));
                        else if (*it < 1.0)
                                c.setHsv(95 - int(*it * 90.0), 255, 255);
                        else
                                c = Qt::red;

                        p.setPen(c);
                        p.drawPoint(x, y--);
                }
        } else {
                p.setPen(QColor(0x1F, 0x20, 0x50));
                p.drawLine(width() - 1, 0, width() - 1, height() - 1);
        }
}


void Sonogram::modifyScope( float *front )
{
        m_fht.power( front );
        m_fht.scale( front, 1.0 / 64 );
}
