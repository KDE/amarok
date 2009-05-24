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
#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsTextItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QScrollArea>
#include <Phonon>

#include <KColorScheme>
#include <kstandarddirs.h>
#include <KVBox>
#include <widgets/kratingpainter.h>
#include <widgets/kratingwidget.h>
#include <plasma/widgets/frame.h>


using namespace std;

VideoclipApplet::VideoclipApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , EngineObserver( The::engineController() )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( false );

    m_height=300;
//    m_mediaObject = const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() );
}


void VideoclipApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    // Load pixmap
    m_pixYoutube = new QPixmap( KStandardDirs::locate("data","amarok/images/amarok-videoclip-youtube.png" ) );
    m_pixDailymotion = new QPixmap( KStandardDirs::locate("data", "amarok/images/amarok-videoclip-dailymotion.png" ) );
    m_pixVimeo = new QPixmap( KStandardDirs::locate("data", "amarok/images/amarok-videoclip-vimeo.png" ) );
    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new QGraphicsSimpleTextItem( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Video Clip" ) );

    // Create layout
    m_layout=new QHBoxLayout();
    m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_layout->setContentsMargins(5,5,5,5);
    m_layout->setSpacing(2);

    // create a widget
    QWidget *window = new QWidget;
    window->setAttribute(Qt::WA_NoSystemBackground);
    window->setLayout(m_layout);

    // create a scroll Area
    QScrollArea *m_scroll=new QScrollArea();
    m_scroll->setMaximumHeight(m_height-m_headerText->boundingRect().height()-4*standardPadding());
    m_scroll->setWidget(window);
    m_scroll->setAttribute(Qt::WA_NoSystemBackground);
    m_scroll->viewport()->setAttribute(Qt::WA_NoSystemBackground);

    m_widget = new QGraphicsProxyWidget(this);
    m_widget->setWidget(m_scroll);

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
    m_widget->resize(size().width()-2 * standardPadding(), size().height()-m_headerText->boundingRect().height()-2*standardPadding());
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
}



QSizeF VideoclipApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
 //   DEBUG_BLOCK
    // hardcoding for now
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), m_height );
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
    
    int width = 130;
    // Properly delete previsouly allocated item
    while(!m_layoutWidgetList.empty())
    {
        m_layoutWidgetList.front()->hide();
        m_layout->removeWidget(m_layoutWidgetList.front());
        m_layoutWidgetList.pop_front();
    }

    // if we get a message, show it
    if( data.contains( "message" ) )
    {
        QLabel *mess = new QLabel(data["message"].toString());
        m_layout->addWidget(mess, Qt::AlignTop);
        m_layoutWidgetList.push_back(mess);
    }
    if (data.contains( "title" ))
    {
        for (int i=0; i<data["title"].toStringList().size();i++)
        {
            debug() << "VideoclipApplet::dataUpdated: "<< data["id"].toStringList().at(i);
            
            // create image
            QLabel *icon = new QLabel();
            icon->setPixmap(The::svgHandler()->addBordersToPixmap(
            data["coverpix"].toHash()[data["cover"].toStringList().at(i)].value<QPixmap>(), 5, "Thumbnail", true ).scaledToHeight(90));
            icon->setToolTip(QString("<html><body><a href=\"")+data["fulllink"].toStringList().at(i)+
            QString("\">")+data["fulllink"].toStringList().at(i)+QString("</a><br>")+data["description"].toStringList().at(i));

            // create link (and resize, no more than 3 lines long)
            QString title(data["title"].toStringList().at(i));
            if (title.size()>45) title.resize(45);
            QLabel *link = new QLabel(QString("<html><body><a href=\"")+data["id"].toStringList().at(i)+QString("\">")+title+QString("</a>"));
            link->setOpenExternalLinks(true);
            link->setWordWrap(true);

            QLabel *duration =  new QLabel(data["duration"].toStringList().at(i)+QString("<br>")+data["views"].toStringList().at(i)+QString(" views"));

            KRatingWidget* rating = new KRatingWidget;
            rating->setRating( (int)(data["rating"].toStringList().at(i).toFloat()*2.));
            rating->setMaximumWidth((int)((width/3)*2));
            rating->setMinimumWidth((int)((width/3)*2));

            QLabel *webi=new QLabel;
            if (data["id"].toStringList().at(i).contains("youtube"))
                webi->setPixmap(*m_pixYoutube);
            else if (data["id"].toStringList().at(i).contains("dailymotion"))
                webi->setPixmap(*m_pixDailymotion);
            else if (data["id"].toStringList().at(i).contains("vimeo"))
                webi->setPixmap(*m_pixVimeo);


            QGridLayout *grid = new QGridLayout;
            grid->setHorizontalSpacing(5);
            grid->setVerticalSpacing(2);
            grid->setRowMinimumHeight(1,70);
            grid->setColumnStretch(0,0);
            grid->setColumnStretch(1,1);
            grid->addWidget(icon, 0,0,1,-1,Qt::AlignCenter);
            grid->addWidget(link, 1,0,1,-1, Qt::AlignCenter | Qt::AlignTop);
            grid->addWidget(webi, 2,0, Qt::AlignCenter);
            grid->addWidget(duration, 2,1, Qt::AlignLeft);
            grid->addWidget(rating, 3,0,1,-1, Qt::AlignCenter);

            // Add The Widget
            QWidget *widget=new QWidget();
            widget->setLayout(grid);
            widget->resize(width, m_height-m_headerText->boundingRect().height()-2*standardPadding());
            widget->setMaximumWidth(width);
            widget->setMinimumWidth(width);
            widget->setMinimumHeight(m_height-(m_headerText->boundingRect().height() + 10 * standardPadding()));
            widget->setMaximumHeight(m_height-(m_headerText->boundingRect().height() + 10 * standardPadding()));
            m_layout->addWidget(widget, Qt::AlignLeft);
            m_layoutWidgetList.push_back(widget);

            if (i<data["title"].toStringList().size()-1)
            {
                QFrame *line= new QFrame();
                line->setFrameStyle(QFrame::VLine);
                line->setAutoFillBackground(false);
                line->setMaximumHeight(m_height-(m_headerText->boundingRect().height() + 2 * standardPadding()));
                m_layout->addWidget(line, Qt::AlignLeft);
                m_layoutWidgetList.push_back(line);
            }
        }
    }
    updateConstraints();
}



 //   setPreferredSize( 500, 300 );
//     //prepare a video widget but hide it
//     QWidget* view = Context::ContextView::self()->viewport();
//      m_videoWidget = new Phonon::VideoWidget();
//      m_videoWidget->setParent( view, Qt::SubWindow | Qt::FramelessWindowHint );
//      m_videoWidget->hide();
//    Phonon::Path path = Phonon::createPath( m_mediaObject, m_videoWidget );
//    if( !path.isValid() )  warning() << "Phonon path is invalid.";


//     for (int i=0; i<data["title"].toStringList().size();i++)
//     {
//         debug() << "VideoclipApplet::title: " << data[ "title" ].toStringList().at(i);
//         debug() << "VideoclipApplet::id: " << data[ "id" ].toStringList().at(i);
//          debug() << "VideoclipApplet::cover: " << data[ "cover" ].toStringList().at(i);
//          debug() << "VideoclipApplet::duration: " << data[ "duration" ].toStringList().at(i);
//          debug() << "VideoclipApplet::views: " << data[ "views" ].toStringList().at(i);
//          debug() << "VideoclipApplet::description: " << data[ "description" ].toStringList().at(i);
//     }


 /*   if (i==0)
        {
            player=true;
                debug() << "VideoclipApplet::dataUpdated: SIMON "<< data["id"].toStringList().at(i);
            m_videoWidget = new Phonon::VideoWidget( this->view()->viewport());
            //  QWidget* view =self()->viewport();
            m_videoWidget->resize(200, 150);
            m_videoWidget->setScaleMode(Phonon::VideoWidget::FitInView);
            m_videoWidget->setFocusPolicy(Qt::StrongFocus);
            m_videoWidget->setParent( this->view()->viewport());//group->nativeWidget());
            grid->addWidget(m_videoWidget,0,1);

            m_mediaObject=new Phonon::MediaObject(this);
            m_mediaObject->setCurrentSource(Phonon::MediaSource("/home/simon/video.flv"));


            Phonon::Path path = Phonon::createPath( m_mediaObject, m_videoWidget );

            m_videoWidget->show();
            m_mediaObject->play();

            if( !path.isValid() ) { debug() << "Phonon path is invalid."; }
            else { debug()<<"Path is valid, where else is the pb ...t"; }
        }*/


    /*    if (title.size()>45) title.resize(45);
        if (title.size()<15)
            link = new QLabel(QString("<html><body><a href=\"")+data["id"].toStringList().at(i)+QString("\">")
            +title+QString("</a>"));//("</a><br><br>"));
        else if (title.size()<30)
            link = new QLabel(QString("<html><body><a href=\"")+data["id"].toStringList().at(i)+QString("\">")
            +title.mid(0,15)+QString("<br>")+title.mid(15,30)+QString("</a><br>"));
        else
            link = new QLabel(QString("<html><body><a href=\"")+data["id"].toStringList().at(i)+QString("\">")
            +title.mid(0,15)+QString("<br>")+title.mid(15,30)+QString("<br>")+title.mid(30,44)+QString("</a>"));*/
//        link->setMaximumWidth(110);
 
#include "VideoclipApplet.moc"

