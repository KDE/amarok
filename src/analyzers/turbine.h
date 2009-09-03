/****************************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>                         *
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

// Amarok BarAnalyzer 3 - Jet Turbine: Symmetric version of analyzer 1

#ifndef ANALYZER_TURBINE_H
#define ANALYZER_TURBINE_H

#include "boomanalyzer.h"

class TurbineAnalyzer : public BoomAnalyzer
{
    public:
        TurbineAnalyzer( QWidget *parent ) : BoomAnalyzer( parent ) {}

        void analyze( const Scope& );
};

#endif
