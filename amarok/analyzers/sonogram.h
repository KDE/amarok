//
//
// C++ Interface: Sonogram
//
// Description: 
//
//
// Author: Melchior FRANZ <mfranz@kde.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SONOGRAM_H
#define SONOGRAM_H

#include "analyzerbase.h"


/**
@author Melchior FRANZ
*/

class Sonogram : public AnalyzerBase
{
    Q_OBJECT

    public:
	Sonogram( QWidget *parent=0, const char *name=0 );
	virtual ~Sonogram();

	virtual void init();
	virtual void drawAnalyzer( std::vector<float> * );

    protected:
	QPixmap *m_pPixmap;
};

#endif
