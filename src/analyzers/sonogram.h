/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
//
// C++ Interface: Sonogram
//
// Description:
//
//
// Author: Melchior FRANZ <mfranz@kde.org>, (C) 2004
//

#ifndef SONOGRAM_H
#define SONOGRAM_H

#include "analyzerbase.h"
//Added by qt3to4:
#include <QResizeEvent>

/**
@author Melchior FRANZ
*/

class Sonogram : public Analyzer::Base2D
{
public:
    Sonogram(QWidget*);
    ~Sonogram();

protected:
    void init();
    void analyze(const Scope&);
    void transform(Scope&);
    void demo();
    void resizeEvent(QResizeEvent*);
    virtual void paintEvent( QPaintEvent * );

    private:
        Scope m_scope;
};

#endif
