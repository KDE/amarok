// Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
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


static inline uint myMax( uint v1, uint v2 ) { return v1 > v2 ? v1 : v2; }

namespace amaroK { extern KConfig *config( const QString& ); }


BlockAnalyzer::BlockAnalyzer( QWidget *parent )
   : Analyzer::Base2D( parent, 20, 9 )
   , m_glow( 1, 1 )
   , m_store( 1 << 8, 0 )      //vector<uint>
   , m_scope( MIN_COLUMNS )    //Scope
   , m_columns( 0 )            //uint
   , m_rows( 0 )               //uint
{
    changeTimeout( amaroK::config( "General" )->readEntry( "Timeout", 9 ) );

    setMinimumSize( MIN_COLUMNS*(WIDTH+1) -1, MIN_ROWS*(HEIGHT+1) -1 ); //-1 is padding, no drawing takes place there
    setMaximumWidth( MAX_COLUMNS*(WIDTH+1) -1 );
}

BlockAnalyzer::~BlockAnalyzer()
{
    amaroK::config( "General" )->writeEntry( "Timeout", timeout() );
}

void
BlockAnalyzer::resizeEvent( QResizeEvent *e )
{
   QWidget::resizeEvent( e );

   canvas()->resize( size() );
   background()->resize( size() );

   const uint oldRows = m_rows;

   //all is explained in analyze()..
   //+1 to counter -1 in maxSizes, trust me we need this!
   m_columns = myMax( uint(double(width()+1) / (WIDTH+1)), MAX_COLUMNS );
   m_rows    = uint(double(height()+1) / (HEIGHT+1));

   m_scope.resize( m_columns );

   if( m_rows != oldRows ) {
      m_glow.resize( WIDTH, m_rows*(HEIGHT+1) );
      m_yscale.resize( m_rows + 1 );

      const uint PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat

      for( uint z = 0; z < m_rows; ++z )
         m_yscale[z] = 1 - (log10( PRE+z ) / log10( PRE+m_rows+PRO ));

      m_yscale[m_rows] = 0;

      paletteChange( palette() );
   }
   else if( width() > e->oldSize().width() || height() > e->oldSize().height() ) {
      //FIXME this block duplicated in paletteChange()

      background()->fill( backgroundColor() );

      const QColor bg = backgroundColor().dark( 112 );
      QPainter p( background() );
      for( int x = 0; (uint)x < m_columns; ++x )
         for( int y = 0; (uint)y < m_rows; ++y )
            p.fillRect( x*(WIDTH+1), y*(HEIGHT+1), WIDTH, HEIGHT, bg );

      bitBlt( canvas(), 0, 0, background() );
   }

   bitBlt( this, 0, 0, background() );
}

void
BlockAnalyzer::transform( Analyzer::Scope &s ) //pure virtual
{
    for( uint x = 0; x < s.size(); ++x )
        s[x] *= 2;

    float *front = static_cast<float*>( &s.front() );

    m_fht->spectrum( front );
    m_fht->scale( front, 1.0 / 20 );

    //the second half is pretty dull, so only show it if the user has a large analyzer
    //by setting to m_scope.size() if large we prevent interpolation of large analyzers, this is good!
    s.resize( m_scope.size() <= MAX_COLUMNS/2 ? MAX_COLUMNS/2 : m_scope.size() );
}

void
BlockAnalyzer::analyze( const Analyzer::Scope &s )
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

   //m_yscale looks similar to: { 0.7, 0.5, 0.25, 0.15, 0.1, 0 }
   //if it contains 6 elements there are 5 rows in the analyzer


   bitBlt( canvas(), 0, 0, background() );

   Analyzer::interpolate( s, m_scope );

   for( uint y, x = 0; x < m_scope.size(); ++x )
   {
      //determine y
      for( y = 0; m_scope[x] < m_yscale[y]; ++y )
          ;

      //too high is not fatal
      //higher than stored value means we are falling
      //fall gradually
      //m_store is twice size of regular units so falling is slower
      if( y * 2 > m_store[x] )
         y = ++m_store[x] / 2 ;
      else
         m_store[x] = y * 2;

      //we start bltting from the top and go down
      //so blt blanks first, then blt glow blocks
      //REMEMBER: y is a number from 0 to m_rows, 0 means all blocks are glowing, m_rows means none are
      bitBlt( canvas(), x*(WIDTH+1), y*(HEIGHT+1), glow(), 0, y*(HEIGHT+1) );
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

      //TODO check if this is necessary
      if( s1 < 0 ) s1 = 0;
      if( s1 > 255 ) s1 = 255;

      //if s1 is invalid, Qt will adjust it
      //but it is not possible to increase the contrast any more, we have done our best

      return QColor( h, s1, v1, QColor::Hsv );
   }

   return c1;
}

void
BlockAnalyzer::paletteChange( const QPalette& ) //virtual
{
   //Qt calls this function when the palette is changed

   const QColor bg = backgroundColor().dark( 112 );
   const QColor fg = ensureContrast( KGlobalSettings::activeTitleColor(), bg );

   {
      const double dr = 15*double(bg.red()   - fg.red())   / (m_rows*16);
      const double dg = 15*double(bg.green() - fg.green()) / (m_rows*16);
      const double db = 15*double(bg.blue()  - fg.blue())  / (m_rows*16);
      const int r = fg.red(), g = fg.green(), b = fg.blue();

      glow()->fill( backgroundColor() );

      QPainter p( glow() );
      for( int y = 0; (uint)y < m_rows; ++y )
         //graduate the fg color
         p.fillRect( 0, y*(HEIGHT+1), WIDTH, HEIGHT, QColor( r+int(dr*y), g+int(dg*y), b+int(db*y) ) );
   }

   {
      background()->fill( backgroundColor() );

      QPainter p( background() );
      for( int x = 0; (uint)x < m_columns; ++x )
         for( int y = 0; (uint)y < m_rows; ++y )
            p.fillRect( x*(WIDTH+1), y*(HEIGHT+1), WIDTH, HEIGHT, bg );

      setErasePixmap( *background() );
   }

   bitBlt( canvas(), 0, 0, background() );
}

void
BlockAnalyzer::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::RightButton )
    {
        //this is hard to read in order to be compact, apologies..
        //the id of each menu item is the value of the attribute it represents,
        //so mapping is concise.

        const uint ids[] = { 50, 33, 25, 20, 10 };

        KPopupMenu menu;
        menu.insertTitle( i18n( "Framerate" ) );

        for( uint x = 0; x < 5; ++x )
        {
            const uint v = ids[x];

            menu.insertItem( i18n( "%1 fps" ).arg( 1000/v ), v );
            menu.setItemChecked( v, v == timeout() );
        }

        const int id = menu.exec( e->globalPos() );

        if ( id != -1 )
            changeTimeout( id );
    }
    else
        e->ignore();
}
