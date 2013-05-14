/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "AnalyzerBase.h"

class QMouseEvent;
class QPalette;
class QResizeEvent;

class BlockAnalyzer : public Analyzer::Base2D
{
public:
    BlockAnalyzer( QWidget* );
    ~BlockAnalyzer();

    // Signed ints because most of what we compare them against are ints
    static const int HEIGHT      = 2;
    static const int WIDTH       = 4;
    static const int MIN_ROWS    = 30;  //arbitrary
    static const int MIN_COLUMNS = 32;  //arbitrary
    static const int MAX_COLUMNS = 256; //must be 2**n
    static const int FADE_SIZE   = 90;

protected:
    virtual void transform( QVector<float>& );
    virtual void analyze( const QVector<float>& );
    virtual void paintEvent( QPaintEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void paletteChange( const QPalette& );

    void drawBackground();
    void determineStep();

private:
    QPixmap* bar()
    {
        return &m_barPixmap;
    }

    uint m_columns, m_rows;      //number of rows and columns of blocks
    uint m_y;                    //y-offset from top of widget
    QPixmap m_barPixmap;
    QPixmap m_topBarPixmap;
    QVector<float> m_scope;      //so we don't create a vector every frame
    std::vector<float> m_store;  //current bar heights
    std::vector<float> m_yscale;

    std::vector<QPixmap> m_fade_bars;
    std::vector<uint>    m_fade_pos;
    std::vector<int>     m_fade_intensity;
    QPixmap              m_background;

    float m_step; //rows to fall per frame
};

#endif
