/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
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

#ifndef BOOMANALYZER_H
#define BOOMANALYZER_H

#include "analyzerbase.h"
//Added by qt3to4:
#include <QPixmap>

/**
@author Max Howell
*/

class BoomAnalyzer : public Analyzer::Base2D
{
Q_OBJECT
public:
    BoomAnalyzer( QWidget* );

    virtual void init();
    virtual void transform( Scope &s );
    virtual void analyze( const Scope& );

public slots:
    void changeK_barHeight( int );
    void changeF_peakSpeed( int );

protected:
    static const uint COLUMN_WIDTH = 4;
    static const uint BAND_COUNT = 32;

    double K_barHeight, F_peakSpeed, F;

    std::vector<float> bar_height;
    std::vector<float> peak_height;
    std::vector<float> peak_speed;

    QPixmap barPixmap;
};

namespace Amarok
{
    namespace ColorScheme
    {
        extern QColor Base;
        extern QColor Text;
        extern QColor Background;
        extern QColor Foreground;
    }
}

#endif
