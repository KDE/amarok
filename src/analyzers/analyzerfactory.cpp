/****************************************************************************************
 * Copyright (c) 2002 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include <config-amarok.h>   //for HAVE_QGLWIDGET macro

#include "amarokconfig.h"
#include "analyzerbase.h" //declaration here

#include "baranalyzer.h"
#include "boomanalyzer.h"
#include "sonogram.h"
#include "turbine.h"
#include "blockanalyzer.h"

#ifdef HAVE_QGLWIDGET
#include "glanalyzer.h"
#include "glanalyzer2.h"
#include "glanalyzer3.h"
#endif

#include <QLabel>
#include <klocale.h>

//separate from analyzerbase.cpp to save compile time



QWidget *Analyzer::Factory::createPlaylistAnalyzer( QWidget *parent)
{
    //FIXME Analyzers are temporarily disabled due to performance issues
    return new QWidget( parent );
    
#if 0
    return new BarAnalyzer( parent );


    QWidget *analyzer = 0;
    switch( AmarokConfig::currentPlaylistAnalyzer() )
    {
        case 1:
            analyzer = new BarAnalyzer( parent );
        case 2:
#if 0
            analyzer = new TurbineAnalyzer( parent );
            break;
#endif
        case 3:
            analyzer = new Sonogram( parent );
            break;
        case 4:
#if 0
            analyzer = new BoomAnalyzer( parent );
            break;
#endif
    #ifdef HAVE_QGLWIDGET
        case 5:
            analyzer = new GLAnalyzer( parent );
            break;
        case 6:
            analyzer = new GLAnalyzer2( parent );
            break;
        case 7:
            analyzer = new GLAnalyzer3( parent );
            break;
        case 8:
    #else
        case 5:
    #endif
            analyzer = new QLabel( i18n( "Click for Analyzers" ), parent ); //blank analyzer to satisfy Grue
            static_cast<QLabel *>(analyzer)->setAlignment( Qt::AlignCenter );
            break;

        default:
            AmarokConfig::setCurrentPlaylistAnalyzer( 0 );
        case 0:
            analyzer = new BlockAnalyzer( parent );
            break;
    }
    return analyzer;
#endif
}
