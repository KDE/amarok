/***************************************************************************
 *   Plasma applet for showing videoclip in the context view.              *
 *                                                                         *
 *   Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "VideoclipApplet.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/ContextView.h"
#include "context/Svg.h"

#include <plasma/theme.h>
#include "SvgHandler.h"

#include <QPainter>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <Phonon>

VideoclipApplet::VideoclipApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , EngineObserver( The::engineController() )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( false );
    m_mediaObject = const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() );
}


void VideoclipApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new QGraphicsSimpleTextItem( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Video Clip" ) );
    
 //   setPreferredSize( 500, 300 );
//     //prepare a video widget but hide it
//     QWidget* view = Context::ContextView::self()->viewport();
//     m_videoWidget = new Phonon::VideoWidget();
//     m_videoWidget->setParent( view, Qt::SubWindow | Qt::FramelessWindowHint );
//     m_videoWidget->hide();
//    Phonon::Path path = Phonon::createPath( m_mediaObject, m_videoWidget );
//    if( !path.isValid() )  warning() << "Phonon path is invalid.";

    // Create frame

    


    // Create layout
    m_layout = new QGraphicsLinearLayout( Qt::Vertical);
    m_layout->setSpacing( 3 );
    
    m_widget = new QGraphicsWidget(this);
    m_widget->setLayout(m_layout);

    constraintsEvent();

    connectSource( "videoclip" );
    connect( dataEngine( "amarok-videoclip" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

}

VideoclipApplet::~VideoclipApplet()
{
    DEBUG_BLOCK
//    delete m_videoWidget;
} 

void EngineNewTrackPlaying()
{
    DEBUG_BLOCK
}

void VideoclipApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();
    
    m_headerText->setPos( size().width() / 2 - m_headerText->boundingRect().width() / 2, standardPadding() + 3 );
    m_widget->setPos(standardPadding(), m_headerText->pos().y() + m_headerText->boundingRect().height() + standardPadding());
    m_widget->resize(size().width()-2 * standardPadding(), size().height()-m_headerText->boundingRect().height()-standardPadding());
}

void VideoclipApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );
    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_headerText );
    
    this->show();
    
}

QSizeF VideoclipApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
 //   DEBUG_BLOCK
    // hardcoding for now
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), 300 );
}


void VideoclipApplet::connectSource( const QString &source )
{
    if( source == "videoclip" )
        dataEngine( "amarok-videoclip" )->connectSource( "videoclip", this );
}

void VideoclipApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( name )
  //  debug() << "VideoclipApplet::dataUpdated: " << name;
    Plasma::GroupBox *g;
    foreach ( g , m_vidGroup)
    {
        g->hide();
    }
    m_vidGroup.clear();


    debug() << "VideoclipApplet::dataUpdated: First update, we don't get any message :" << data["title"].toStringList().size();
    for (int i=0; i<data["title"].toStringList().size();i++)
    {
        Plasma::GroupBox *group=new Plasma::GroupBox(m_widget);
        debug() << "VideoclipApplet::dataUpdated: "<< data["id"].toStringList().at(i);

        QLabel *title=new QLabel(
            QString("<html><body><a href=\"")
            +data["id"].toStringList().at(i)+QString("\">")
            +data["title"].toStringList().at(i)+QString("</a><br>")
            +data["duration"].toStringList().at(i)+QString("<br>")
            +data["views"].toStringList().at(i)+QString(" views<br>")
            +data["rating"].toStringList().at(i)+QString(" / 5</body></html>"));
        title->setOpenExternalLinks(true);

        QLabel *iconLabel=new QLabel();
        iconLabel->setPixmap(The::svgHandler()->addBordersToPixmap(
        data["coverpix"].toHash()[data["cover"].toStringList().at(i)].value<QPixmap>(), 5, "Thumbnail", true ).scaledToHeight(80));
        //iconLabel->resize(120, 90);

        QGridLayout *hbox = new QGridLayout;
        hbox->addWidget(iconLabel, 0,0);
        hbox->addWidget(title, 1,0);
    //    hbox->addWidget(
        //hbox->addStretch(1);
        group->nativeWidget()->setLayout(hbox);

        m_vidGroup.push_back(group);
    }

    //remove everything
    while(m_layout->count()!=0)
        m_layout->removeAt(0);
    for (int i=0; i<m_vidGroup.size();i++)
        m_layout->addItem(m_vidGroup.at(i));

//     for (int i=0; i<data["title"].toStringList().size();i++)
//     {
//         debug() << "VideoclipApplet::title: " << data[ "title" ].toStringList().at(i);
//         debug() << "VideoclipApplet::id: " << data[ "id" ].toStringList().at(i);
//          debug() << "VideoclipApplet::cover: " << data[ "cover" ].toStringList().at(i);
//          debug() << "VideoclipApplet::duration: " << data[ "duration" ].toStringList().at(i);
//          debug() << "VideoclipApplet::views: " << data[ "views" ].toStringList().at(i);
//          debug() << "VideoclipApplet::description: " << data[ "description" ].toStringList().at(i);
//     }
    updateConstraints();
}



#include "VideoclipApplet.moc"

