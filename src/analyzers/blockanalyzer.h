// Maintainer: Max Howell <mac.howell@methylblue.com>, (C) 2003-4
// Copyright:  See COPYING file that comes with this distribution
//

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "analyzerbase.h"

/**
@author Max Howell
*/

class QResizeEvent;
class QMouseEvent;
class QPalette;

class BlockAnalyzer : public Analyzer::Base2D
{
public:
    BlockAnalyzer( QWidget* );
    ~BlockAnalyzer();

    static const uint HEIGHT      = 2;
    static const uint WIDTH       = 4;
    static const uint MIN_ROWS    = 3;   //arbituary
    static const uint MIN_COLUMNS = 32;  //arbituary
    static const uint MAX_COLUMNS = 256; //must be 2**n

protected:
    virtual void transform( Scope& );
    virtual void analyze( const Scope& );
    virtual void resizeEvent( QResizeEvent* );
    virtual void mousePressEvent( QMouseEvent* );
    virtual void paletteChange( const QPalette& );

    void drawBackground();

private:
    QPixmap* const glow() { return &m_glow; }

    uint m_columns, m_rows;    //current size values
    uint m_y;                  //y-offset
    QPixmap m_glow;
    Scope m_scope;             //so we don't create a vector every frame
    std::vector<uint> m_store; //store previous values
    std::vector<float> m_yscale;
};

#endif
