// (c) Max Howell 2004
// See COPYING file for licensing information

#include "playlistbrowser.h"
#include "metabundle.h"   //prettyLength()
#include "threadweaver.h" //PLStats Job

#include <qevent.h>      //customEvent()
#include <qfontmetrics.h>//paintItem()
#include <qpainter.h>    //paintItem()
#include <kiconloader.h> //smallIcon
#include <klocale.h>
#include <kurldrag.h>    //dragObject()



PlaylistBrowser::PlaylistBrowser( const char *name )
   : KIconView( 0, name )
{
    setResizeMode( QIconView::Adjust );
    setSelectionMode( QIconView::Extended );
    setGridX( 100 );
}

PlaylistBrowser::~PlaylistBrowser()
{}

QDragObject*
PlaylistBrowser::dragObject()
{
    return new KURLDrag( currentItem()->url(), this );
}

void
PlaylistBrowser::customEvent( QCustomEvent *e )
{
    if( e->type() != ThreadWeaver::Job::PLStats ) return;

    #define e static_cast<PLStats*>(e)
    Item *item = new PlaylistBrowser::Item( this, e->url(), e->contents(), e->length() );
    #undef e
}



PlaylistBrowser::Item::Item( QIconView *parent, const KURL &u, const KURL::List &list, const uint length )
   : KIconViewItem( parent )
   , m_url( u )
   , m_numberTracks( list.count() )
   , m_length( MetaBundle::prettyLength( length ) )
{
    QString name = u.fileName();
    setText( fileBasename( name ) );
    setDragEnabled( true );
    setPixmap( findCoverArt( list.first() ) );
}

void
PlaylistBrowser::Item::paintItem( QPainter *p, const QColorGroup &cg )
{
    QIconViewItem::paintItem( p, cg );

    QFontMetrics fm( iconView()->font() );
    p->setPen( Qt::green );
    QRect r = textRect( false );
    p->drawText( r.left(), r.bottom(), metaString() );
}
#include <kdebug.h>
void
PlaylistBrowser::Item::calcRect( const QString &s )
{
    const QFontMetrics fm( iconView()->font() );
    const uint width = fm.width( metaString() );
    QRect r = textRect( true );
    r.setWidth( width );
    setTextRect( r );

    KIconViewItem::calcRect( s );

    r = textRect( true );
    r.rBottom() += fm.height();
    setTextRect( r );
//kdDebug() << "text: " << r << endl;
    r = rect();
    r.rBottom() += fm.height();
    r.setWidth( width );
    setItemRect( r );
//kdDebug() << "item: " << r << endl;
}

inline QString
PlaylistBrowser::Item::metaString() const
{
    return i18n( "eg. 3 Tracks - [67:43]", "%1 Tracks - [%2]" ).arg( QString::number( m_numberTracks ), m_length );
}

#include <dirent.h>
#include <qimage.h>
QPixmap PlaylistBrowser::findCoverArt( const KURL &url ) //static
{
    //TODO this function should save the thumbnail to amarok dir too so it is found in this function next time
    //TODO check our thumbnail cache first
    //TODO Playlisttooltip has this code, don't replicate!
    //TODO pixmap generation should be done in the thread

    QStringList validExts;
    validExts << "jpg";
    validExts << "png";
    validExts << "gif";
    validExts << "jpeg";

    DIR *d = opendir( url.directory( FALSE, FALSE ).local8Bit() );
    if( d )
    {
        dirent *ent;
        while( (ent = readdir( d )) )
        {
            QString file( ent->d_name );
            QString ext = fileExtension( file );

            if( validExts.contains( ext ) )
            {
                QString
                path  = url.directory( FALSE, FALSE );
                path += file;

                QImage img( path );//, ext.local8Bit() );
                QPixmap pix;
                if( pix.convertFromImage( img.smoothScale( 64, 64 ) ) )
                    return pix;
            }

        }
    }

    return DesktopIcon( "midi" );
}

#include "playlistbrowser.moc"
