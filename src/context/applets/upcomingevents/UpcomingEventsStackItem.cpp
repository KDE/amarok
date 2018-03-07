/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UpcomingEventsStackItem"

#include "UpcomingEventsStackItem.h"
#include "UpcomingEventsStack.h"
#include "core/support/Debug.h"
#include "context/widgets/TextScrollingWidget.h"

#include <KIconLoader>
#include <Plasma/FrameSvg>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <Plasma/PushButton>
#include <Plasma/Theme>

#include <QAction>
#include <QFontMetrics>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneResizeEvent>
#include <QLabel>
#include <QPainter>
#include <QSignalMapper>
#include <QStyleOptionGraphicsItem>
#include <QWeakPointer>

class UpcomingEventsStackItemToolBox : public QGraphicsWidget
{
public:
    UpcomingEventsStackItemToolBox( QGraphicsWidget *parent )
        : QGraphicsWidget( parent )
        , m_background( new Plasma::FrameSvg(this) )
    {
        m_background->setImagePath( "widgets/extender-dragger" );
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
        updateTheme();
    }

    qreal iconSize()
    {
        return m_iconSize;
    }

    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * )
    {
        m_background->paintFrame( painter, option->exposedRect, option->exposedRect );
    }

    void updateTheme()
    {
        //Read the preferred icon size hint, look at the font size, and calculate the desired title bar
        //icon height.
        m_background->resize();
        QSizeF size = m_background->elementSize( "hint-preferred-icon-size" );
        size = size.expandedTo( QSizeF(KIconLoader::SizeSmall,KIconLoader::SizeSmall) );

        Plasma::Theme *theme = Plasma::Theme::defaultTheme();
        QFont font = theme->font( Plasma::Theme::DefaultFont );
        QFontMetrics fm( font );
        m_iconSize = qMax( size.height(), (qreal) fm.height() );
    }

    void setBackgroundPrefix( const QString &string )
    {
        if( string.isEmpty() || m_background->hasElementPrefix(string) )
        {
            m_background->setElementPrefix( string );
            update();
        }
    }

    const QString backgroundPrefix() const
    {
        return m_background->prefix();
    }

protected:
    void resizeEvent( QGraphicsSceneResizeEvent *event )
    {
        Q_UNUSED( event )
        m_background->resizeFrame( size() );
    }

private:
    Plasma::FrameSvg *m_background;
    QString m_prefix;
    qreal m_iconSize;
};

class UpcomingEventsStackItemPrivate
{
private:
    UpcomingEventsStackItem *const q_ptr;
    Q_DECLARE_PUBLIC( UpcomingEventsStackItem )

public:
    UpcomingEventsStackItemPrivate( UpcomingEventsStackItem *parent );
    ~UpcomingEventsStackItemPrivate();

    Plasma::IconWidget *collapseButton;
    Plasma::IconWidget *destroyButton;
    bool destroyButtonEnabled;
    QHash<QString, QAction*> actions;
    QSignalMapper *maximizeSignalMapper;

    bool collapsed;
    QGraphicsLinearLayout *layout;
    QGraphicsLinearLayout *toolboxLayout;
    QString name;
    QString title;
    QString iconName;
    QWeakPointer<QGraphicsWidget> widget;
    TextScrollingWidget *titleLabel;
    UpcomingEventsStack *stack;
    UpcomingEventsStackItemToolBox *toolbox;

    void _themeChanged();
    void _toggleCollapse();
    void _updateToolbox();
};

UpcomingEventsStackItemPrivate::UpcomingEventsStackItemPrivate( UpcomingEventsStackItem *parent )
    : q_ptr( parent )
    , collapseButton( 0 )
    , destroyButton( 0 )
    , destroyButtonEnabled( false )
    , maximizeSignalMapper( 0 )
    , collapsed( false )
    , layout( 0 )
    , toolboxLayout( 0 )
    , titleLabel( 0 )
    , stack( 0 )
    , toolbox( 0 )
{
}

UpcomingEventsStackItemPrivate::~UpcomingEventsStackItemPrivate()
{
}

void
UpcomingEventsStackItemPrivate::_themeChanged()
{
    toolbox->updateTheme();
}

void
UpcomingEventsStackItemPrivate::_toggleCollapse()
{
    Q_Q( UpcomingEventsStackItem );
    q->setCollapsed( !q->isCollapsed() );
}

void
UpcomingEventsStackItemPrivate::_updateToolbox()
{
    Q_ASSERT( toolbox );
    Q_ASSERT( toolboxLayout );
    Q_Q( UpcomingEventsStackItem );
    const int startingIndex = 2; // collapse item is index 0, title label is 1
    const QSizeF widgetSize = collapseButton->sizeFromIconSize( toolbox->iconSize() );
    titleLabel->setText( title );

    QHash<QAction *, QGraphicsWidget *> actionWidgets;
    for( int index = startingIndex; index < toolboxLayout->count(); ++index)
    {
        QGraphicsWidget *widget = dynamic_cast<QGraphicsWidget*>( toolboxLayout->itemAt(index) );
        QAction *widgetAction = 0;

        if( !widget )
            continue;
        else if( qobject_cast<Plasma::IconWidget*>(widget) )
            widgetAction = static_cast<Plasma::IconWidget*>( widget )->action();
        else if( qobject_cast<Plasma::PushButton*>(widget) )
            widgetAction = static_cast<Plasma::PushButton*>( widget )->action();
        else
        {
            toolboxLayout->removeAt(index);
            widget->deleteLater();
        }
        
        if( widget != destroyButton )
            actionWidgets.insert( widgetAction, widget );
    }

    // ensure the collapseButton is the correct size.
    collapseButton->setMinimumSize( widgetSize );
    collapseButton->setMaximumSize( widgetSize );

    // add the actions that are actually set to visible.
    foreach( QAction *action, actions.values() )
    {
        if( action->isVisible() )
        {
            Plasma::IconWidget *icon = qobject_cast<Plasma::IconWidget*>( actionWidgets.value(action) );
            Plasma::PushButton *button = qobject_cast<Plasma::PushButton*>( actionWidgets.value(action) );

            if( action->icon().isNull() && !action->text().isNull() )
            {
                if( !button )
                {
                    button = new Plasma::PushButton;
                    button->setAction( action );
                }
                button->setMinimumHeight( widgetSize.height() );
                button->setMaximumHeight( widgetSize.height() );
                button->setCursor( Qt::ArrowCursor );
                toolboxLayout->insertItem( startingIndex, button );
            }
            else
            {
                if( !icon )
                {
                    icon = new Plasma::IconWidget;
                    icon->setAction( action );
                }
                if( action->icon().isNull() )
                    icon->setText( action->text() );

                icon->setMinimumSize( widgetSize );
                icon->setMaximumSize( widgetSize );
                icon->setCursor( Qt::ArrowCursor );
                toolboxLayout->insertItem( startingIndex, icon );
            }
        }
    }

    // add destroy button last
    if( destroyButtonEnabled )
    {
        if( !destroyButton )
        {
            QAction *closeAction = new QAction( q );
            destroyButton = new Plasma::IconWidget( toolbox );
            destroyButton->setAction( closeAction );
            destroyButton->setSvg( QLatin1String("widgets/configuration-icons"), QLatin1String("close") );
            destroyButton->setMinimumSize( widgetSize );
            destroyButton->setMaximumSize( widgetSize );
            destroyButton->setCursor( Qt::ArrowCursor );
            QObject::connect( closeAction, SIGNAL(triggered()), q, SLOT(deleteLater()) );
        }
        toolboxLayout->addItem( destroyButton );
    }
    toolboxLayout->invalidate();
}

UpcomingEventsStackItem::UpcomingEventsStackItem( const QString &name,
                                                  UpcomingEventsStack *parent)
    : QGraphicsWidget( parent )
    , d_ptr( new UpcomingEventsStackItemPrivate( this ) )
{
    Q_ASSERT( parent );
    Q_D( UpcomingEventsStackItem );
    d->stack = parent;
    d->name = name;

    //create the toolbox.
    d->toolbox = new UpcomingEventsStackItemToolBox( this );
    d->toolboxLayout = new QGraphicsLinearLayout( d->toolbox );

    //create own layout
    d->layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    d->layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->layout->addItem( d->toolbox );
    d->layout->setContentsMargins( 0, 0, 0, 0 );

    //create maximize action
    d->maximizeSignalMapper = new QSignalMapper( d->toolbox );
    connect( d->maximizeSignalMapper, SIGNAL(mapped(QString)), d->stack, SLOT(maximizeItem(QString)) );

    Plasma::Svg svg;
    svg.setImagePath( QLatin1String("widgets/configuration-icons") );
    QAction *maximizeAction = new QAction( svg.pixmap(QLatin1String("restore")), QString(), d->toolbox );
    maximizeAction->setToolTip( i18n( "Maximize" ) );
    connect( maximizeAction, SIGNAL(triggered()), d->maximizeSignalMapper, SLOT(map()) );
    d->maximizeSignalMapper->setMapping( maximizeAction, d->name );
    d->actions.insert( QLatin1String("maximize"), maximizeAction );

    d->collapseButton = new Plasma::IconWidget( d->toolbox );
    d->collapseButton->setCursor( Qt::ArrowCursor );
    d->titleLabel = new TextScrollingWidget( d->toolbox );
    d->titleLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    d->toolboxLayout->addItem( d->collapseButton );
    d->toolboxLayout->addItem( d->titleLabel );
    d->toolboxLayout->setStretchFactor( d->titleLabel, 10 );
    connect( d->collapseButton, SIGNAL(clicked()), SLOT(_toggleCollapse()) );

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setContentsMargins( 0, 0, 0, 0 );

    d->_updateToolbox();
    d->_themeChanged();
    connect( Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), SLOT(_themeChanged()) );
}

UpcomingEventsStackItem::~UpcomingEventsStackItem()
{
    delete d_ptr;
}

void
UpcomingEventsStackItem::setWidget( QGraphicsWidget *widget )
{
    Q_ASSERT( widget );
    Q_D( UpcomingEventsStackItem );
    if( d->widget.data() )
    {
        d->layout->removeItem( d->widget.data() );
        delete d->widget.data();
    }
    widget->setParentItem( this );
    d->widget = widget;
    d->layout->insertItem( 1, d->widget.data() );
    d->layout->setItemSpacing( 0, 2 );
    d->widget.data()->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->widget.data()->setVisible( !d->collapsed );
}

QGraphicsWidget *
UpcomingEventsStackItem::widget() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->widget.data();
}

void
UpcomingEventsStackItem::setTitle( const QString &title )
{
    Q_D( UpcomingEventsStackItem );
    d->title = title;
    d->_updateToolbox();
}

QString
UpcomingEventsStackItem::title() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->title;
}

void
UpcomingEventsStackItem::setName( const QString &name )
{
    Q_D( UpcomingEventsStackItem );
    d->name = name;
}

void
UpcomingEventsStackItem::addAction( const QString &name, QAction *action )
{
    Q_ASSERT( action );
    Q_D( UpcomingEventsStackItem );
    d->actions.insert( name, action );
    d->_updateToolbox();
}

QHash<QString, QAction *>
UpcomingEventsStackItem::actions() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->actions;
}

QAction *
UpcomingEventsStackItem::action( const QString &name ) const
{
    Q_D( const UpcomingEventsStackItem );
    return d->actions.value( name );
}

QString
UpcomingEventsStackItem::name() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->name;
}

void
UpcomingEventsStackItem::setIcon( const QIcon &icon )
{
    Q_D( UpcomingEventsStackItem );
    d->collapseButton->setIcon( icon );
    d->collapseButton->setVisible( !icon.isNull() );
    d->_updateToolbox();
}

void
UpcomingEventsStackItem::setIcon( const QString &icon )
{
    Q_D( UpcomingEventsStackItem );
    if( icon != d->iconName )
    {
        d->collapseButton->setIcon( icon );
        d->iconName = icon;
    }
}

QIcon
UpcomingEventsStackItem::icon() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->collapseButton->icon();
}

UpcomingEventsStack *
UpcomingEventsStackItem::stack() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->stack;
}

bool
UpcomingEventsStackItem::isCollapsed() const
{
    Q_D( const UpcomingEventsStackItem );
    return d->collapsed;
}

void
UpcomingEventsStackItem::setCollapsed( bool collapsed )
{
    Q_D( UpcomingEventsStackItem );
    d->collapsed = collapsed;
    if( d->widget )
    {
        prepareGeometryChange();
        d->widget.data()->setVisible( !collapsed );
        if( collapsed )
            d->layout->removeItem( d->widget.data() );
        else
        {
            d->layout->insertItem( 1, d->widget.data() );
            d->layout->setItemSpacing( 0, 2 );
        }
        d->toolboxLayout->invalidate();
        emit collapseChanged( collapsed );
        updateGeometry();
    }
    d->collapseButton->setToolTip( collapsed ? i18n("Expand this widget") : i18n("Collapse this widget") );
}

void
UpcomingEventsStackItem::showCloseButton( bool show )
{
    Q_D( UpcomingEventsStackItem );
    d->destroyButtonEnabled = show;
    d->_updateToolbox();
}

void
UpcomingEventsStackItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event )
{
    Q_D( UpcomingEventsStackItem );
    if( d->toolbox->boundingRect().contains( event->pos() ) )
        d->_toggleCollapse();
}

void
UpcomingEventsStackItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_D( UpcomingEventsStackItem );
    if( !(d->toolbox->boundingRect().contains(event->pos())) )
        event->ignore();
}

QSizeF
UpcomingEventsStackItem::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_D( const UpcomingEventsStackItem );
    QSizeF size = d->toolbox->effectiveSizeHint( which, constraint );
    if( !d->collapsed && d->widget )
    {
        size.rheight() += d->layout->itemSpacing( 1 ) * 2;
        size.rheight() += d->widget.data()->effectiveSizeHint( which, constraint ).height();
    }
    return size;
}

QRectF
UpcomingEventsStackItem::boundingRect() const
{
    Q_D( const UpcomingEventsStackItem );
    if( !d->collapsed && d->widget )
    {
        QSharedPointer<QGraphicsWidget> w = d->widget.toStrongRef();
        if( w )
            return d->toolbox->boundingRect().united( mapRectFromItem( w.data(), w->boundingRect() ) );
    }
    return d->toolbox->boundingRect();
}

