#include "AudioHostListItem.h"

#include <qapplication.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qslider.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>

AudioHostListItem::AudioHostListItem( bool valid, QString _hostname, QWidget *parent )
    : QWidget( parent )
{
    /* layout */
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setAutoAdd( false );

    /* status button */
    statusButton = new QLabel(this);
    statusButton->setPixmap( valid ? SmallIcon( "greenled" ) : SmallIcon( "redled" )  );
    l->addWidget(statusButton);
    
    /* host label */
    hostLabel = new QLabel( _hostname, this );
    l->addWidget(hostLabel);

    l->addSpacing(10);
    
    /* volume slider */
    // TODO: dummy slider, create own volume slider
    QSlider *slider = new QSlider( -100, 100, 10, 0, Qt::Horizontal, this );
    slider->setValue(50);
    l->addWidget(slider);

    l->addStretch(1);

    setHighlighted( false );
}

AudioHostListItem::~AudioHostListItem()
{
}

void AudioHostListItem::setHighlighted( bool highlight )
{
    if( highlight )
        setPaletteBackgroundColor( calcBackgroundColor( "activeBackground", QApplication::palette().active().highlight() ) );
    else
        setPaletteBackgroundColor( calcBackgroundColor( "windowBackground", Qt::white ) );
}

QString AudioHostListItem::hostname() const
{ 
    return hostLabel->text();
}

void AudioHostListItem::mousePressEvent( QMouseEvent * )
{
    emit pressed( this );
}

QColor AudioHostListItem::calcBackgroundColor( QString type, QColor color )
{
    KConfig *config = KGlobal::config();
    config->setGroup("WM");
    return config->readColorEntry( type, &color);
}

#include "AudioHostListItem.moc"
