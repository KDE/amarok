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

using Analyzer::Scope;

/**
@author Melchior FRANZ
*/

class Sonogram : public Analyzer::Base2D
{
public:
	Sonogram(QWidget*);
	~Sonogram();

	void init();
	void analyze(const Scope&);
	void transform(Scope&);
	void demo();
};

#endif
