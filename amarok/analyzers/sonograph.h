//
//
// C++ Interface: Sonograph
//
// Description: 
//
//
// Author: Melchior FRANZ <mfranz@kde.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SONOGRAPH_H
#define SONOGRAPH_H

#include "analyzerbase.h"


/**
@author Melchior FRANZ
*/

class Sonograph : public AnalyzerBase
{
    Q_OBJECT

    public:
	Sonograph( QWidget *parent=0, const char *name=0 );
	virtual ~Sonograph();

	virtual void init();
	virtual void drawAnalyzer( std::vector<float> * );

    protected:
	QPixmap *m_pPixmap;
};

#endif
