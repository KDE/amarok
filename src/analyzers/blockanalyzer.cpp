// Author:    Max Howell <max.howell@methylblue.com>, (C) 2003
// Copyright: See COPYING file that comes with this distribution
//

#include "blockanalyzer.h"
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h> //paletteChange()
#include <klocale.h>         //mousePressEvent
#include <kpopupmenu.h>      //mousePressEvent
#include <math.h>            //resizeEvent()
#include <qevent.h>          //mousePressEvent


namespace amaroK { extern KConfig *config( const QString& ); }

static float lvlMapper[BlockAnalyzer::MAX_ROWS+1];


BlockAnalyzer::BlockAnalyzer( QWidget *parent )
 : Analyzer::Base2D( parent, 20, 8 )
 , m_dark( WIDTH, HEIGHT )   //QPixmap
 , m_store( 2 << 8, 0 )      //vector<uint>
 , m_scope( MIN_COLUMNS )    //Scope
 , m_columns( MIN_COLUMNS )  //uint
 , m_rows( MIN_ROWS )        //uint
{
    for( uint x = 0; x < MAX_ROWS; ++x ) m_glow[x].resize( WIDTH, HEIGHT );

    changeTimeout( amaroK::config( "General" )->readNumEntry( "Timeout", 20 ) );

    setMinimumSize( MIN_COLUMNS*(WIDTH+1) -1, MIN_ROWS*(HEIGHT+1) -1 ); //-1 is padding, no drawing takes place there
    setMaximumSize( MAX_COLUMNS*(WIDTH+1) -1, MAX_ROWS*(HEIGHT+1) -1 );
}

BlockAnalyzer::~BlockAnalyzer()
{
    amaroK::config( "General" )->writeEntry( "Timeout", timeout() );
}


static inline uint limit( uint val, uint max, uint min ) { return val < min ? min : val > max ? max : val; }

uint /*ox,*/ oy;

void
BlockAnalyzer::resizeEvent( QResizeEvent *e )
{
    Analyzer::Base2D::resizeEvent( e );

    //all is explained in analyze()..

    m_columns = limit( uint(double(width()+1) / (WIDTH+1)), MAX_COLUMNS, MIN_COLUMNS ); //+1 to counter -1 in maxSizes, trust me we need this!
    m_rows    = limit( uint(double(height()+1) / (HEIGHT+1)), MAX_ROWS, MIN_ROWS );

    m_scope.resize( m_columns );

    if( (uint)e->oldSize().height() != height() ) //this block speeds up window resizes vastly
    {
        //NOTE height should only be set once! but it tends to get set many times when Qt is setting up
        //     the layout of the toolBar, what a waste of cycles!

        const uint PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat
        for( uint z = 0; z < m_rows; ++z )
        {
            lvlMapper[z] = 1-(log10(PRE+z) / log10(PRE+m_rows+PRO));
        }
        lvlMapper[m_rows] = 0;

        //for( uint x = 0; x <= m_rows; ++x ) kdDebug() << x << ": " << endl;

        paletteChange( palette() );

        //ox = uint(((width()%(WIDTH+1))-1)/2); //TODO make member // -1 due to margin on right in draw routine
        oy = height()%(HEIGHT+1);             //TODO make member
    }
}

void
BlockAnalyzer::transform( Scope &s )
{
    float *front = static_cast<float*>( &s.front() );

    m_fht.spectrum( front );
    m_fht.scale( front, 1.0 / 20 );

    //the second half is pretty dull, so only show it if the user has a large analyzer
    //by setting to m_scope.size() if large we prevent interpolation of large analyzers, this is good!
    s.resize( m_scope.size() <= MAX_COLUMNS/2 ? MAX_COLUMNS/2 : m_scope.size() );
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

    Scope &v = m_scope;
    uint z;

    Analyzer::interpolate( s, v );

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

        //FIXME less blits is better, so store the whole bar as one pixmap and blt however much you need
        //FIXME the background pixmap should be the all the dark squares

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

void
BlockAnalyzer::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::RightButton )
    {
        //this is hard to read in order to be compact, apologies..
        //the id of each menu item is the value of the attribute it represents,
        //so mapping is concise.

        const uint ids[4] = { 50, 33, 25, 20 };

        KPopupMenu menu;
        menu.insertTitle( i18n( "Framerate" ) );

        for( uint x = 0; x < 4; ++x )
        {
            const uint v = ids[x];

            menu.insertItem( i18n( "%1 fps" ).arg( 1000/v ), v );
            menu.setItemChecked( v, v == timeout() );
        }

        const int id = menu.exec( e->globalPos() );
        if( id >= 20 ) changeTimeout( id );
    }
}

static QColor
ensureContrast( const QColor &c1, const QColor &c2, uint amount = 150 )
{
    //TODO together with the contrast functions in PlaylistItem, make a separate header for these

    int h, s1, v1, s2, v2;

    c2.hsv( &h, &s2, &v2 );
    c1.hsv( &h, &s1, &v1 ); //must be second, see setHsv() later

    const uint contrast = abs(v2 - v1) + abs(s2 - s1);

    if( amount > contrast )
    {
        //TODO this algorythm is basic but simple,
        //it doesn't handle all cases
        //I can almost see a complex but compact full solution, keep at it!

        const uint difference = amount - contrast;
        uint error = 0;

        v1 += (v2 <= v1) ? difference : -difference;

        if( v1 > 255 )
        {
            error = v1 - 255;
            v1 = 255;
        }

        if( v1 < 0 )
        {
            error = -v1;
            v1 = 0;
        }

        if( error != 0 ) s1 += (s2 < s1) ? error : -error;

        //TODO check if this is necessary
        if( s1 < 0 ) s1 = 0;
        if( s1 > 255 ) s1 = 255;

        //if s1 is no invalid, Qt will adjust it
        //but it is not possible to increase the contrast any more, we have done our best

        return QColor( h, s1, v1, QColor::Hsv );
    }

    return c1;
}

void
BlockAnalyzer::paletteChange( const QPalette &p )
{
    //Qt calls this function when the palette is changed

    const QColor bg = backgroundColor().dark( 112 );
    const QColor fg = ensureContrast( KGlobalSettings::activeTitleColor(), bg );

    const double dr = 15*double(bg.red()   - fg.red())   / (m_rows*16);
    const double dg = 15*double(bg.green() - fg.green()) / (m_rows*16);
    const double db = 15*double(bg.blue()  - fg.blue())  / (m_rows*16);

    const int r = fg.red(), g = fg.green(), b = fg.blue();

    for( int x = 0; (uint)x < m_rows; ++x )
    {
        m_glow[x].fill( QColor( r+int(dr*x), g+int(dg*x), b+int(db*x) ) ); //graduate the fg color
    }

    m_dark.fill( bg );

    Base2D::paletteChange( p );
}
