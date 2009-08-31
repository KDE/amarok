/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "analyzerbase.h"
#include <QColor>
//Added by qt3to4:
#include <QResizeEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPixmap>

class QResizeEvent;
class QMouseEvent;
class QPalette;


/**
 * @author Max Howell
 */

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
    static const uint FADE_SIZE   = 90;

protected:
    virtual void transform( Scope& );
    virtual void analyze( const Scope& );
    virtual void paintEvent( QPaintEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void contextMenuEvent( QContextMenuEvent* );
    virtual void paletteChange( const QPalette& );

    void drawBackground();
    void determineStep();

private:
    QPixmap* const bar() { return &m_barPixmap; }

    uint m_columns, m_rows;      //number of rows and columns of blocks
    uint m_y;                    //y-offset from top of widget
    QPixmap m_barPixmap;
    QPixmap m_topBarPixmap;
    Scope m_scope;               //so we don't create a vector every frame
    std::vector<float> m_store;  //current bar heights
    std::vector<float> m_yscale;

    //FIXME why can't I namespace these? c++ issue?
    std::vector<QPixmap> m_fade_bars;
    std::vector<uint>    m_fade_pos;
    std::vector<int>     m_fade_intensity;

    float m_step; //rows to fall per frame
};

#endif
