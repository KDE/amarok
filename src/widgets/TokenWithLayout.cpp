/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Roman Jarosz <kedgedev@gmail.com>                                 *
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

#include "TokenWithLayout.h"

#include "Debug.h"
#include "TokenDropTarget.h"

#include <KAction>
#include <KColorScheme>
#include <KHBox>
#include <KIcon>
#include <KLocale>

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QLayout>
#include <QLCDNumber>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QTimerEvent>

Wrench::Wrench( QWidget *parent ) : QLabel( parent )
{
    setCursor( Qt::ArrowCursor );
    setPixmap( KIcon( "configure" ).pixmap( 64 ) );
    setScaledContents( true );
    setMargin( 4 );
}

void Wrench::enterEvent( QEvent * )
{
    setMargin( 1 );
    update();
}

void Wrench::leaveEvent( QEvent * )
{
    setMargin( 4 );
    update();
}

void Wrench::mouseReleaseEvent( QMouseEvent * )
{
    emit clicked();
}

void Wrench::paintEvent( QPaintEvent *pe )
{
    QPainter p( this );
    QColor c = palette().color( backgroundRole() );
//     if ( underMouse() )
//         c = KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color();
//     p.setPen( QPen ( c, 2 ) );
    p.setPen( Qt::NoPen );
    c = palette().color( backgroundRole() );
    c.setAlpha( 212 );
    p.setBrush( c );
    p.setRenderHint( QPainter::Antialiasing );
    p.drawEllipse( rect() );
    p.end();
    QLabel::paintEvent( pe );
}


const QString ActionBoldName = QLatin1String( "ActionBold" );
const QString ActionItalicName = QLatin1String( "ActionItalic" );
const QString ActionAlignLeftName = QLatin1String( "ActionAlignLeft" );
const QString ActionAlignCenterName = QLatin1String( "ActionAlignCenter" );
const QString ActionAlignRightName = QLatin1String( "ActionAlignRight" );

Token * TokenWithLayoutFactory::createToken( const QString &text, const QString &iconName, int value, QWidget *parent )
{
    return new TokenWithLayout( text, iconName, value, parent );
}

TokenWithLayout::TokenWithLayout( const QString &text, const QString &iconName, int value, QWidget *parent )
    : Token( text, iconName, value, parent  )
    , m_width( 0.0 ), m_wrenchTimer( 0 )
{
    m_widthForced = m_width > 0.0;
    m_alignment = Qt::AlignCenter;
    m_bold = false;
    m_italic = false;
    m_wrench = new Wrench( this );
    m_wrench->installEventFilter( this );
    m_wrench->hide();
    connect ( m_wrench, SIGNAL( clicked() ), this, SLOT( showConfig() ) );
    setFocusPolicy( Qt::ClickFocus );
}


TokenWithLayout::~TokenWithLayout()
{
    delete m_wrench;
}

void TokenWithLayout::enterEvent( QEvent *e )
{
    QWidget *win = window();
    const int sz = 2*height();
    QPoint pt = mapTo( win, rect().topLeft() );

    m_wrench->setParent( win );
    m_wrench->setFixedSize( sz, sz );
    m_wrench->move( pt - QPoint( m_wrench->width()/3, m_wrench->height()/3 ) );
    m_wrench->setCursor( Qt::PointingHandCursor );
    m_wrench->raise();
    m_wrench->show();

    Token::enterEvent( e );
}

bool TokenWithLayout::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Leave && o == m_wrench )
    {
        if ( m_wrenchTimer )
            killTimer( m_wrenchTimer );
        m_wrenchTimer = startTimer( 40 );
    }
    return false;
}

void TokenWithLayout::fillMenu( QMenu * menu )
{
    DEBUG_BLOCK
    KAction *boldAction = new KAction( KIcon( "format-text-bold"), i18n( "Bold" ), menu );
    boldAction->setObjectName( ActionBoldName );
    boldAction->setCheckable( true );
    boldAction->setChecked( m_bold );

    KAction *italicAction = new KAction( KIcon( "format-text-italic"), i18n( "Italic" ), menu );
    italicAction->setObjectName( ActionItalicName );
    italicAction->setCheckable( true );
    italicAction->setChecked( m_italic );

    KAction *alignLeftAction = new KAction( KIcon( "format-justify-left"), i18n( "Left" ), menu );
    KAction *alignCenterAction = new KAction( KIcon( "format-justify-center"), i18n( "Center" ), menu );
    KAction *alignRightAction = new KAction( KIcon( "format-justify-right"), i18n( "Right" ), menu );
    alignLeftAction->setObjectName( ActionAlignLeftName );
    alignLeftAction->setCheckable( true );
    alignCenterAction->setObjectName( ActionAlignCenterName );
    alignCenterAction->setCheckable( true );
    alignRightAction->setObjectName( ActionAlignRightName );
    alignRightAction->setCheckable( true );

    if ( m_alignment & Qt::AlignLeft )
        alignLeftAction->setChecked( true );
    else if ( m_alignment & Qt::AlignHCenter )
        alignCenterAction->setChecked( true );
    else if ( m_alignment & Qt::AlignRight )
        alignRightAction->setChecked( true );

    QActionGroup *alignmentGroup = new QActionGroup( menu );
    alignmentGroup->addAction( alignLeftAction );
    alignmentGroup->addAction( alignCenterAction );
    alignmentGroup->addAction( alignRightAction );

    menu->addAction( boldAction );
    menu->addAction( italicAction );
    menu->addSeparator()->setText( i18n( "Alignment" ) );
    menu->addAction( alignLeftAction );
    menu->addAction( alignCenterAction );
    menu->addAction( alignRightAction );
    menu->addSeparator()->setText( i18n( "Width" ) );
    menu->adjustSize();

    int orgHeight = menu->height();

    KHBox * sliderBox = new KHBox( menu );
    sliderBox->setFixedWidth( menu->width() - 4 );
    sliderBox->move( sliderBox->pos().x() + 2, orgHeight );

    QSlider * slider = new QSlider( Qt::Horizontal, sliderBox );
    slider->setMaximum( 100 );
    slider->setMinimum( 0 );

    // this should really not be done here as it makes upward assumptions
    // it was however done in setWidth with similar upward assumptions as well
    // solution: the popup stuff -iff- should be done in the dialog or the editWidget
    if ( parentWidget() )
    {
        if ( TokenDropTarget *editWidget = qobject_cast<TokenDropTarget*>( parentWidget() ) )
        {
            qreal spareWidth = 100.0;
            int row = editWidget->row( this );
            if ( row > -1 )
            {
                QList<Token*> tokens = editWidget->drags( row );
                foreach ( Token *t, tokens )
                {
                    if ( t == this )
                        continue;
                    if ( TokenWithLayout *twl = qobject_cast<TokenWithLayout*>( t ) )
                        spareWidth -= twl->width() * 100.0;
                }
            }

            int max = qMax<qreal>( spareWidth, 0.0 );
            debug() << "slider max value: " << max;

            if ( max >= m_width * 100.0 )
                slider->setMaximum( qMax<qreal>( spareWidth, 0.0 ) );
            else
                slider->setMaximum( m_width * 100.0 );
        }
    }
    slider->setValue( m_width * 100.0 );

    QLCDNumber * sizeLabel = new QLCDNumber( 3, sliderBox );
    sizeLabel->display( m_width * 100.0 );

    connect( slider, SIGNAL( valueChanged( int ) ), sizeLabel, SLOT( display( int ) ) );
    connect( slider, SIGNAL( valueChanged( int ) ), this, SLOT( setWidth( int ) ) );

    menu->setFixedHeight( orgHeight + slider->height() );

}

void TokenWithLayout::leaveEvent( QEvent *e )
{
    Token::leaveEvent( e );
    if ( m_wrenchTimer )
        killTimer( m_wrenchTimer );
    m_wrenchTimer = startTimer( 40 );
}

void TokenWithLayout::menuExecuted( const QAction* action )
{
    if( action->objectName() == ActionAlignLeftName )
        setAlignment( Qt::AlignLeft );
    else if( action->objectName() == ActionAlignCenterName )
        setAlignment( Qt::AlignCenter );
    else if( action->objectName() == ActionAlignRightName )
        setAlignment( Qt::AlignRight );
    else if( action->objectName() == ActionBoldName )
        setBold( action->isChecked() );
    else if( action->objectName() == ActionItalicName )
        setItalic( action->isChecked() );
}

// temp flag used to make the popup behave more like a toolbutton popup, can be removed w/ dialog replacement
// code tagged "temp.popup"
static bool configMenuVisible = false; // "temp.popup"

void TokenWithLayout::showConfig()
{
    QMenu* menu = new QMenu();

    menu->setTitle( i18n( "Layout" ) );

    fillMenu( menu );

    configMenuVisible = true; // "temp.popup"
    QAction* action = menu->exec( QCursor::pos() - QPoint( menu->width()/2, -2 ) );
    if ( action )
        menuExecuted( action );
    configMenuVisible = false; // "temp.popup"

    delete menu;

    QTimerEvent te( m_wrenchTimer );
    timerEvent( &te ); // "temp.popup"
}

void TokenWithLayout::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == m_wrenchTimer )
    {
        killTimer( m_wrenchTimer );
        m_wrenchTimer = 0;
        if ( !configMenuVisible ) // "temp.popup"
        {
            QRegion rgn;
            rgn |= QRect( mapToGlobal( QPoint( 0, 0 ) ), QWidget::size() );
            rgn |= QRect( m_wrench->mapToGlobal( QPoint( 0, 0 ) ), m_wrench->size() );
            if ( !rgn.contains( QCursor::pos() ) )
                m_wrench->hide();
        }
    }
    Token::timerEvent( te );
}

Qt::Alignment TokenWithLayout::alignment()
{
    return m_alignment;
}

void TokenWithLayout::setAlignment( Qt::Alignment alignment )
{
    if ( m_alignment == alignment )
        return;

    m_alignment = alignment;
    m_label->setAlignment( alignment );
    emit changed();
}

void TokenWithLayout::setAlignLeft( bool b )
{
    if (b)
        setAlignment( Qt::AlignLeft );
}

void TokenWithLayout::setAlignCenter( bool b )
{
    if (b)
        setAlignment( Qt::AlignCenter );
}

void TokenWithLayout::setAlignRight( bool b )
{
    if (b)
        setAlignment( Qt::AlignRight );
}

bool TokenWithLayout::bold() const
{
    return m_bold;
}

void TokenWithLayout::setBold( bool bold )
{
    if ( m_bold == bold )
        return;

    m_bold = bold;
    QFont font = m_label->font();
    font.setBold( bold );
    m_label->setFont( font );
    emit changed();
}

void TokenWithLayout::setPrefix( const QString& string )
{
    if ( m_prefix == string )
        return;

    m_prefix = string;
    emit changed();
}

void TokenWithLayout::setSuffix( const QString& string )
{
    if ( m_suffix == string )
        return;

    m_suffix = string;
    emit changed();
}

void TokenWithLayout::setWidth( int size )
{
    m_width = qMax( qMin( 1.0, size/100.0 ), 0.0 ) ;
    if ( m_width > 0.0 )
        m_widthForced = true;

    emit changed();
}

void TokenWithLayout::setWidthForced( bool on )
{
    m_widthForced = on;
}

qreal TokenWithLayout::width() const
{
    return m_width;
}

bool TokenWithLayout::italic() const
{
    return m_italic;
}

void TokenWithLayout::setItalic( bool italic )
{
    if ( m_italic == italic )
        return;

    m_italic = italic;
    QFont font = m_label->font();
    font.setItalic( italic );
    m_label->setFont( font );

    emit changed();
}


#include "TokenWithLayout.moc"



