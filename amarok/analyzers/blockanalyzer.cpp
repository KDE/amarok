//
// C++ Implementation: blockanalyzer
//
// Description:
//
//
// Author: Max Howell <max.howell@methylblue.com>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "blockanalyzer.h"
#include "fht.h"
#include <kdebug.h>
#include <math.h>
#include <qevent.h>


static float lvlMapper[BlockAnalyzer::MAX_ROWS+1];// = { 0.080, 0.140, 0.200, 0.300, 0.500, 0.700, 100 };


BlockAnalyzer::BlockAnalyzer( QWidget *parent )
 : Analyzer::Base2D( parent, 20, 8 )
 , m_dark( WIDTH, HEIGHT )   //QPixmap
 , m_store( MAX_COLUMNS, 0 ) //vector<uint>
 , m_scope( MIN_COLUMNS )    //Scope
 , m_columns( MIN_COLUMNS )  //uint
{
    QColor darkColor( backgroundColor().dark( 150 ) );

    m_dark.fill( darkColor );

    setMinimumSize( MIN_COLUMNS*(WIDTH+1) -1, MIN_ROWS*(HEIGHT+1) -1 ); //-1 is padding that isn't drawn to
    setMaximumSize( MAX_COLUMNS*(WIDTH+1) -1, MAX_ROWS*(HEIGHT+1) -1 );
}

static inline uint limit( uint val, uint max, uint min ) { return val < min ? min : val > max ? max : val; }

uint /*ox,*/ oy;

void
BlockAnalyzer::resizeEvent( QResizeEvent *e )
{
    //all is explained in analyze()..

    m_columns = limit( uint(double(width()+1) / (WIDTH+1)), MAX_COLUMNS, MIN_COLUMNS ); //+1 to counter -1 in maxSizes, trust me we need this!
    m_rows    = limit( uint(double(height()+1) / (HEIGHT+1)), MAX_ROWS, MIN_ROWS );

    m_scope.resize( m_columns );


    const uint PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat
    for( uint z = 0; z < m_rows; ++z )
    {
        lvlMapper[z] = 1-(log10(PRE+z) / log10(PRE+m_rows+PRO));
    }
    lvlMapper[m_rows] = 0;

    //for( uint x = 0; x <= m_rows; ++x ) kdDebug() << x << ": " << lvlMapper[x] << "\n";


    QColor darkColor( backgroundColor().dark( 150 ) );

     double dr = 7.5*double(darkColor.red()   - 32) / (m_rows*8);
     double dg = 7.5*double(darkColor.green() - 32) / (m_rows*8);
     double db = 7.5*double(darkColor.blue()  - 82) / (m_rows*8);

    for( uint x = 0; x < m_rows; ++x )
    {
        m_glow[x].resize( WIDTH, HEIGHT );
        m_glow[x].fill( QColor( 32+int(dr*x), 32+int(dg*x), 82+int(db*x) ) ); //amaroK blue, graduated
    }


    //ox = uint(((width()%(WIDTH+1))-1)/2); //TODO make member // -1 due to margin on right in draw routine
    oy = height()%(HEIGHT+1);             //TODO make member

    Analyzer::Base2D::resizeEvent( e );
}

void
BlockAnalyzer::transform( Scope &s )
{
    float *front = static_cast<float*>( &s.front() );

    m_fht.spectrum( front );
    m_fht.scale( front, 1.0 / 20 );

    s.resize( MAX_COLUMNS );
}

void
BlockAnalyzer::analyze( const Scope &s )
{
    //static float max = 0;

    // z = 2 3 2 1 0 2
    //     . . . . # .
    //     . . . # # .
    //     # . # # # #
    //     # # # # # #
    //
    // visual aid for how this analyzer works.
    // z represents the number of blanks
    // z starts from the top and increases in units of blocks

    //lvlMapper looks similar to: { 0.7, 0.5, 0.25, 0.15, 0.1, 0 }
    //if it contains 6 elements there are 5 rows in the analyzer

    Analyzer::interpolate( s, m_scope );

    Scope &v = m_scope;
    uint z;

    for( uint x = 0; x < v.size(); ++x )
    {
        for( z = 0; v[x] < lvlMapper[z]; ++z );

        //this is debug stuff
        //if( v[x] > max ) { max = v[x]; kdDebug() << max << endl; }

        //too high is not fatal
        //higher than stored value means we are falling
        //fall gradually
        //m_store is twice size of regular units so falling is slower
        if( z * 2 > m_store[x] )
        {
            z = ++m_store[x] / 2 ;
        }
        else m_store[x] = z * 2;

        //TODO try just drawing blocks with Qt, then X functions
        //     I reckon that will be quicker coz Qt's bitBlt is an OGRE of a function!

        //NOTE actually, tests show that all the cpu being used is for the FHT, these blts are insiginificant
        //     still it would be trivial to only blt changes, so do that.

        //we start bltting from the top and go down
        //so blt blanks first, then blt glow blocks
        //REMEMBER: z is a number from 0 to m_rows, 0 means all blocks are glowing, m_rows means none are
        for( uint y = 0; y < m_rows; ++y ) //TODO only blt the bits that have changed
        {
            if( y >= z ) //greater than z means we blt bottom, ie glow blocks
                bitBlt( canvas(), x*(WIDTH+1), y*(HEIGHT+1) + oy, &m_glow[y] ); //x+1 = margin
            else
                bitBlt( canvas(), x*(WIDTH+1), y*(HEIGHT+1) + oy, &m_dark );
        }

    }
}
