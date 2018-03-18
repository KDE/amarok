/****************************************************************************************
 * Copyright (c) 2014 Matej Repinc <mrepinc@gmail.com>                                  *
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

#ifndef ASCIIANALYZER_H
#define ASCIIANALYZER_H

#include "AnalyzerBase.h"

#include <QImage>
#include <QPixmap>
#include <QSharedPointer>
#include <QSize>

class QMouseEvent;
class QPalette;
class QResizeEvent;

class ASCIIAnalyzer : public Analyzer::Base
{
public:
    explicit ASCIIAnalyzer( QWidget* );

    static GLuint createTexture( const QImage &image ) { return instance->bindTexture( image ); }
    static void freeTexture( GLuint id ) { instance->deleteTexture( id ); }

    // Signed ints because most of what we compare them against are ints
    static const int BLOCK_HEIGHT = 12;
    static const int BLOCK_WIDTH  = 12;
    static const int MIN_ROWS     = 30;  //arbitrary
    static const int MIN_COLUMNS  = 32;  //arbitrary
    static const int MAX_COLUMNS  = 128; //must be 2**n
    static const int FADE_SIZE    = 90;

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL( int w, int h );
    virtual void transform( QVector<float>& );
    virtual void analyze( const QVector<float>& );
    virtual void paletteChange( const QPalette& );

    void drawBackground();
    void determineStep();

private:
    struct Texture
    {
        Texture( const QPixmap &pixmap ) :
            id( ASCIIAnalyzer::createTexture( pixmap.toImage().mirrored() ) ), // Flip texture vertically for OpenGL bottom-left coordinate system
            size( pixmap.size() )
        {}
        Texture( const Texture& texture )
        {
            id = texture.id;
            size = texture.size;
        }
        ~Texture()
        {
            ASCIIAnalyzer::freeTexture( id );
        }

        GLuint id;
        QSize size;
    };

    void drawTexture( Texture* texture, int x, int y, int sx, int sy );

    static ASCIIAnalyzer* instance;

    int m_columns, m_rows;      //number of rows and columns of blocks
    QPixmap m_barPixmap;
    QVector<float> m_scope;      //so we don't create a vector every frame
    QVector<float> m_store;  //current bar heights
    QVector<float> m_yscale;

    QSharedPointer<Texture> m_barTexture;
    QSharedPointer<Texture> m_topBarTexture;
    QSharedPointer<Texture> m_topSecondBarTexture;
    QSharedPointer<Texture> m_background;

    float m_step; //rows to fall per frame
};

#endif
