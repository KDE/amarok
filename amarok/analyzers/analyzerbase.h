/***************************************************************************
                          viswidget.h  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANALYZERBASE_H
#define ANALYZERBASE_H

#include <vector>

class QWidget;

#define SINVEC_SIZE 6000

/**
 *@author Max, Piggz
 */

class AnalyzerBase
{
public:
    virtual ~AnalyzerBase();

    virtual void drawAnalyzer( std::vector<float>* ) = 0;
    uint timeout() const { return m_timeout; }

protected:
    AnalyzerBase( uint );

    void interpolate( std::vector<float>*, std::vector<float>& ) const;
    virtual void init() = 0;

private:
    uint m_timeout;

public:

    class AnalyzerFactory
    {
        //Currently this is a rather small class, its only purpose
        //to ensure that making changes to analyzers will not require
        //rebuilding the world!

        //eventually it would be better to make analyzers pluggable
        //but I can't be arsed, nor can I see much reason to do so
        //yet!
    public:
        static AnalyzerBase *createAnalyzer( QWidget* );
    };
};

#endif
