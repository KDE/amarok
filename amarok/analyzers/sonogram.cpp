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


void Sonogram::analyze(const Scope &s)
{
	int x = width() - 1;
	QColor c;
	QPainter p(canvas());

	bitBlt(canvas(), 0, 0, canvas(), 1, 0, x, height());
	Scope::const_iterator it = s.begin();
	for (int y = height() - 1; y && it < s.end(); it++) {
		if (*it < .005)
			c = backgroundColor();
		else if (*it < .05)
			c.setHsv(95, 255, 255 - int(*it * 4000.0));
		else if (*it < 1.0)
			c.setHsv(95 - int(*it * 90.0), 255, 255);
		else
			c = Qt::red;

		p.setPen(c);
		p.drawPoint(x, y--);
	}
}


void Sonogram::transform(Scope &scope)
{
	float *front = static_cast<float*>(&scope.front());

	m_fht.power(front);
	m_fht.scale(front, 1.0 / 64);
}


void Sonogram::demo()
{
	analyze(Scope(m_fht.size(), 0));
}

