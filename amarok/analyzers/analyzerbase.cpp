/***************************************************************************
                          viswidget.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
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

#include "analyzerbase.h"

#include <math.h>
#include <vector>

// INSTRUCTIONS
// 1. inherit AnalyzerBase( first parameter to AnalyzerBase is the frequency (in milliseconds) that drawAnalyser will be called)
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this at the end of your implementation of drawAnalyser()


AnalyzerBase::AnalyzerBase( uint timeout ) :
   m_timeout( timeout )
{}

AnalyzerBase::~AnalyzerBase()
{}


// METHODS =====================================================

void AnalyzerBase::interpolate( std::vector<float> *oldVec, std::vector<float> &newVec ) const
{
    if ( oldVec->size() )
    {    
        uint newSize = newVec.size(); //vector::size() is O(1)
    
        //necessary? code bloat if not
        if( newSize == oldVec->size() ) { newVec = *oldVec; return; }
    
        double pos = 0.0;
        double step = static_cast<double>( oldVec->size() ) / newSize;
    
        for ( uint i = 0; i < newSize; ++i, pos += step )
        {
            double error = pos - floor( pos );
            unsigned long offset = static_cast<unsigned long>( pos );
    
            unsigned long indexLeft = offset + 0;
    
            if ( indexLeft >= oldVec->size() )
                indexLeft = oldVec->size() - 1;
    
            unsigned long indexRight = offset + 1;
    
            if ( indexRight >= oldVec->size() )
                indexRight = oldVec->size() - 1;
    
            newVec[i] = (*oldVec)[indexLeft] * ( 1.0 - error ) +
                        (*oldVec)[indexRight] * error;
        }
    }
}
