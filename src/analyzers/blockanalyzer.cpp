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
#include <qpainter.h>        //paletteChange()


namespace amaroK { extern KConfig *config( const QString& ); }

static inline uint limit( uint val, uint max, uint min ) { return val < min ? min : val > max ? max : val; }


BlockAnalyzer::BlockAnalyzer( QWidget *parent )
 : Analyzer::Base2D( parent, 20, 8 )
 , m_glow( WIDTH, (HEIGHT+1)*MAX_ROWS ) //QPixmap
 , m_dark( WIDTH, (HEIGHT+1)*MAX_ROWS ) //QPixmap
 , m_store( 2 << 8, 0 )      //vector<uint>
 , m_scope( MIN_COLUMNS )    //Scope
 , m_columns( 0 )            //uint
 , m_rows( 0 )               //uint
 , oy( 0 )                   //uint
{
    changeTimeout( amaroK::config( "General" )->readNumEntry( "Timeout", 20 ) );

    setMinimumSize( MIN_COLUMNS*(WIDTH+1) -1, MIN_ROWS*(HEIGHT+1) -1 ); //-1 is padding, no drawing takes place there
    setMaximumSize( MAX_COLUMNS*(WIDTH+1) -1, MAX_ROWS*(HEIGHT+1) -1 );

    //ensure that paletteChange() gets called via resizeEvent()
    //FIXME Qt should do this! but the very first resizeEvent is with QResizeEvent( size(), size() );
    //TODO  I reported the bug, lets see what happens for Qt 4.0
    QResizeEvent re( size(), QSize() );
    resizeEvent( &re );
}

BlockAnalyzer::~BlockAnalyzer()
{
    amaroK::config( "General" )->writeEntry( "Timeout", timeout() );
}

void
BlockAnalyzer::resizeEvent( QResizeEvent *e )
{
    Analyzer::Base2D::resizeEvent( e );

    //all is explained in analyze()..
    //+1 to counter -1 in maxSizes, trust me we need this!
    m_columns = limit( uint(double(width()+1) / (WIDTH+1)), MAX_COLUMNS, MIN_COLUMNS );
    m_scope.resize( m_columns );

    if( e->oldSize().height() != height() ) //this block speeds up window resizes vastly
    {
        //NOTE height should only be set once! but it tends to get set many times when Qt is setting up
        //     the layout of the toolBar, what a waste of cycles!

        m_rows = limit( uint(double(height()+1) / (HEIGHT+1)), MAX_ROWS, MIN_ROWS );

        const uint PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat
        for( uint z = 0; z < m_rows; ++z )
        {
            yscale[z] = 1-(log10(PRE+z) / log10(PRE+m_rows+PRO));
        }
        yscale[m_rows] = 0;

        paletteChange( palette() );

        oy = height()%(HEIGHT+1);
    }
}

void
BlockAnalyzer::transform( Scope &s )
{
    for( uint x = 0; x < s.size(); ++x )
        s[x] *= 2;

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
    // z = 2 3 2 1 0 2
    //     . . . . # .
    //     . . . # # .
    //     # . # # # #
    //     # # # # # #
    //
    // visual aid for how this analyzer works.
    // z represents the number of blanks
    // z starts from the top and increases in units of blocks

    //yscale looks similar to: { 0.7, 0.5, 0.25, 0.15, 0.1, 0 }
    //if it contains 6 elements there are 5 rows in the analyzer

    Analyzer::interpolate( s, m_scope );

    for( uint z, x = 0; x < m_scope.size(); ++x )
    {
        for( z = 0; m_scope[x] < yscale[z]; ++z );

        //too high is not fatal
        //higher than stored value means we are falling
        //fall gradually
        //m_store is twice size of regular units so falling is slower
        if( z * 2 > m_store[x] )
        {
            z = ++m_store[x] / 2 ;
        }
        else m_store[x] = z * 2;

        //we start bltting from the top and go down
        //so blt blanks first, then blt glow blocks
        //REMEMBER: z is a number from 0 to m_rows, 0 means all blocks are glowing, m_rows means none are
        bitBlt( canvas(), x*(WIDTH+1), oy, dark() );
        bitBlt( canvas(), x*(WIDTH+1), oy + z*(HEIGHT+1), glow(), 0, z*(HEIGHT+1) );
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
    else
        e-> ignore();
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
BlockAnalyzer::paletteChange( const QPalette &palette ) //virtual
{
    //Qt calls this function when the palette is changed

    const QColor bg = backgroundColor().dark( 112 );
    const QColor fg = ensureContrast( KGlobalSettings::activeTitleColor(), bg );

    const double dr = 15*double(bg.red()   - fg.red())   / (m_rows*16);
    const double dg = 15*double(bg.green() - fg.green()) / (m_rows*16);
    const double db = 15*double(bg.blue()  - fg.blue())  / (m_rows*16);

    const int r = fg.red(), g = fg.green(), b = fg.blue();


    m_glow.fill( backgroundColor() );
    m_dark.fill( backgroundColor() );

    QPainter p( &m_glow );
    for( int y = 0; (uint)y < m_rows; ++y )
    {
        //graduate the fg color
        p.fillRect( 0, y*(HEIGHT+1), WIDTH, HEIGHT, QColor( r+int(dr*y), g+int(dg*y), b+int(db*y) ) );
    }
    p.end();

    p.begin( dark() );
    for( int y = 0; (uint)y < m_rows; ++y )
    {
        p.fillRect( 0, y*(HEIGHT+1), WIDTH, HEIGHT, bg );
    }

    Base2D::paletteChange( palette );
}
