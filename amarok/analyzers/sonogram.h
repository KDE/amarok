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

namespace Analyzer {

class Sonogram : public Analyzer::Base2D
{
    public:
        Sonogram( QWidget* );
        ~Sonogram();

        void init();
        void drawAnalyzer( std::vector<float> * );
        void modifyScope( float* );
};

}

#endif
