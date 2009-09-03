/****************************************************************************************
 * Copyright (c) 2004 Melchior Franz <mfranz@kde.org>                                   *
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
