/***************************************************************************
                        analyzerfactory.cpp  -  description
                           -------------------
  begin                : Fre Nov 15 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h> //for HAVE_QGLWIDGET macro

#include "amarokconfig.h"
#include "analyzerbase.h" //declaration here

#include "baranalyzer.h"
#include "distortanalyzer.h"
#include "glanalyzer.h"
#include "glanalyzer2.h"
#include "sonogram.h"
#include "turbine.h"

//FIXME is there a better define?
#define AMAROK_RELEASE

#ifndef AMAROK_RELEASE
    #include "blockanalyzer.h"
    #include "baranalyzer2.h"
    #include "spectralshine.h"
    #include "xmasdrug.h"
#endif

//separate from analyzerbase.cpp to save compile time
//sorry if this isn't to your liking..

//const
AnalyzerBase *AnalyzerBase::AnalyzerFactory::createAnalyzer( QWidget *parent )
{
    AnalyzerBase *analyzer = 0;

    switch( AmarokConfig::currentAnalyzer() )
    {
    case 1:
        analyzer = new DistortAnalyzer( parent );
        break;
    case 2:
        analyzer = new Sonogram( parent );
        break;
    case 3:
        analyzer = new TurbineAnalyzer( parent );
        break;
#ifdef HAVE_QGLWIDGET
    case 4:
        analyzer = new GLAnalyzer( parent );
        break;
    case 5:
        analyzer = new GLAnalyzer2( parent );
        break;
#endif
#ifndef AMAROK_RELEASE
    case 6:
        analyzer = new XmasAnalyzer( parent );
        break;
    case 7:
        analyzer = new BlockAnalyzer( parent );
        break;
    case 8:
        analyzer = new BarAnalyzer2( parent );
        break;
    case 9:
        analyzer = new SpectralShineAnalyzer( this );
        break;
#endif
    default:
        AmarokConfig::setCurrentAnalyzer( 0 );
    case 0:
        analyzer = new BarAnalyzer( parent );
    }

    return analyzer;
}
