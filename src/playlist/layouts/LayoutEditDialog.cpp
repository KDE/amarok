/****************************************************************************************
 * Copyright (c) 2009 Thomas Lübking <thomas.luebking@web.de                            *
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

#include "LayoutEditDialog.h"
#include "TokenWithLayout.h"
#include "TokenDropTarget.h"

#include <KIcon>
#include <KLocale>

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QRadioButton>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QToolButton>

class HintingLineEdit : public QLineEdit
{
public:
    HintingLineEdit( const QString &hint = QString(), QWidget *parent = 0 ) : QLineEdit( parent ), m_hint( hint )
    { }
    void setHint( const QString &hint )
    {
        m_hint = hint;
    }
protected:
    void paintEvent ( QPaintEvent *pe )
    {
        QLineEdit::paintEvent( pe );
        if ( !hasFocus() && text().isEmpty() )
        {
            QStyleOptionFrame opt;
            initStyleOption( &opt );
            
            QPainter p(this);
            QColor fg = palette().color( foregroundRole() );
            fg.setAlpha( fg.alpha() / 2 );
            p.setPen( fg );
            
            p.drawText( style()->subElementRect( QStyle::SE_LineEditContents, &opt, this ),
                                                 alignment() | Qt::TextSingleLine | Qt::TextIncludeTrailingSpaces,
                                                 m_hint );
            p.end();
        }
    }
private:
    QString m_hint;
};

LayoutEditDialog::LayoutEditDialog( QWidget *parent ) : QDialog( parent ), m_token( 0 )
{
    setWindowTitle( i18n( "Configuration for" ) );
    
    QFont boldFont = font();
    boldFont.setBold( true );
    QVBoxLayout *l1 = new QVBoxLayout( this );
    QHBoxLayout *l2 = new QHBoxLayout;
    l2->addWidget( m_prefix = new HintingLineEdit( "[prefix]", this ) );
    m_prefix->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    l2->addWidget( m_element = new QLabel( this ) );
    m_element->setFont( boldFont );
    l2->addWidget( m_suffix = new HintingLineEdit( "[suffix]", this ) );
    l1->addLayout( l2 );

    QFrame *line = new QFrame( this );
    line->setFrameStyle( QFrame::Sunken | QFrame::HLine );
    l1->addWidget( line );

    QWidget *boxWidget = new QWidget( this );
    QLabel *l;

#define HAVE_WIDTH_MODES 0

    QHBoxLayout *l4 = new QHBoxLayout;
    l = new QLabel( i18n( "Width: " ), this );
    l->setFont( boldFont );
    l4->addWidget( l );
    l4->addWidget( m_fixedWidth = new QRadioButton( i18n( "custom" ), this ) );
    m_fixedWidth->setToolTip( i18n( "A fix absolute or relative value (e.g. 128px or 12%)" ) );
    m_fixedWidth->setChecked( true );
#if HAVE_WIDTH_MODES
    l4->addWidget( m_fitContent = new QRadioButton( i18n( "fit content" ), this ) );
    m_fitContent->setToolTip( i18n( "Fit the element text" ) );
#endif
    l4->addWidget( m_peerWidth = new QRadioButton( i18n( "automatic" ), this ) );
    m_peerWidth->setToolTip( i18n( "Take homogeneous part of the space available to all elements with automatic width" ) );
    l4->addStretch();
    boxWidget->connect( m_fixedWidth, SIGNAL( toggled(bool) ), SLOT( setEnabled(bool) ) );
    connect( m_peerWidth, SIGNAL( toggled(bool) ), SLOT( setPeerWidth(bool) ) );
    l1->addLayout( l4 );

    QHBoxLayout *l5 = new QHBoxLayout( boxWidget );
    l5->addWidget( m_width = new QSlider( Qt::Horizontal, boxWidget ) );
    m_width->setRange( 0, 100 );
    l = new QLabel( boxWidget );
    l5->addWidget( l );
//         width->connect( sizeMode, SIGNAL( currentIndexChanged(int) ), SLOT( setDisabled() ) )
    l->setNum( 0 );
    l->connect( m_width, SIGNAL( valueChanged(int) ), SLOT( setNum(int) ) );

#define HAVE_METRICS 0
#if HAVE_METRICS
    QComboBox *metrics = new QComboBox( this );
    metrics->setFrame( false );
    metrics->addItem( "%" );
    metrics->addItem( "px" );
    metrics->addItem( "chars" );
    l5->addWidget( metrics );
#else
    QLabel *metrics = new QLabel( "%", this );
    l5->addWidget( metrics );
#endif

    l1->addWidget( boxWidget );

    line = new QFrame( this );
    line->setFrameStyle( QFrame::Sunken | QFrame::HLine );
    l1->addWidget( line );

    QHBoxLayout *l3 = new QHBoxLayout;
    l = new QLabel( i18n( "Alignment: " ), this );
    l->setFont( boldFont );
    l3->addWidget( l );
    l3->addWidget( m_alignLeft = new QToolButton( this ) );
    l3->addWidget( m_alignCenter = new QToolButton( this ) );
    l3->addWidget( m_alignRight = new QToolButton( this ) );
    
    l3->addSpacing( 12 );

    l = new QLabel( i18n( "Font: " ), this );
    l->setFont( boldFont );
    l3->addWidget( l );
    l3->addWidget( m_bold = new QToolButton( this ) );
    l3->addWidget( m_italic = new QToolButton( this ) );
    l3->addStretch();
    l1->addLayout( l3 );

    QDialogButtonBox *box = new QDialogButtonBox(this);
    box->addButton( QDialogButtonBox::Cancel );
    box->addButton( QDialogButtonBox::Ok );
    connect( box, SIGNAL( rejected() ), SLOT( reset() ) );
    connect( box, SIGNAL( rejected() ), SLOT( close() ) );
    connect( box, SIGNAL( accepted() ), SLOT( close() ) );
    l1->addWidget( box );

    l1->addStretch();

    m_alignLeft->setIcon( KIcon( "format-justify-left" ) );
    m_alignLeft->setCheckable( true );
    m_alignCenter->setIcon( KIcon( "format-justify-center" ) );
    m_alignCenter->setCheckable( true );
    m_alignRight->setIcon( KIcon( "format-justify-right" ) );
    m_alignRight->setCheckable( true );
    QButtonGroup *align = new QButtonGroup( this );
    align->setExclusive( true );
    align->addButton( m_alignLeft );
    align->addButton( m_alignCenter );
    align->addButton( m_alignRight );

    m_bold->setIcon( KIcon( "format-text-bold" ) );
    m_bold->setCheckable( true );
    m_italic->setIcon( KIcon( "format-text-italic" ) );
    m_italic->setCheckable( true );

}

void LayoutEditDialog::reset()
{
    if ( !m_token )
        return;
    m_token->setPrefix( m_originalPrefix );
    m_token->setSuffix( m_originalSuffix );
    m_token->setWidth( m_originalWidth );
    m_token->setAlignment( m_originalAlignment );
    m_token->setBold( m_originalBold );
    m_token->setItalic( m_originalItalic );
}

void LayoutEditDialog::setPeerWidth( bool peer )
{
    if ( peer )
    {
        m_previousWidth = m_width->value();
        m_width->setValue( 0 );
    }
    else
        m_width->setValue( m_previousWidth );
}


void LayoutEditDialog::setToken( TokenWithLayout *t )
{
    if ( m_token )
    {
        m_prefix->disconnect( m_token );
        m_suffix->disconnect( m_token );
        m_width->disconnect( m_token );
        m_alignLeft->disconnect( m_token );
        m_alignCenter->disconnect( m_token );
        m_alignRight->disconnect( m_token );
        m_bold->disconnect( m_token );
        m_italic->disconnect( m_token );
    }
    m_token = t;
    if ( m_token )
    {
        m_element->setText( m_token->name() );
        m_prefix->setText( m_originalPrefix = m_token->prefix() );
        m_suffix->setText( m_originalSuffix = m_token->suffix() );


        // this should still not be done here as it makes upward assumptions
        // solution(?) token->element->row->elements
        if ( m_token->parentWidget() )
        {
            if ( TokenDropTarget *editWidget = qobject_cast<TokenDropTarget*>( m_token->parentWidget() ) )
            {
                qreal spareWidth = 100.0;
                int row = editWidget->row( m_token );
                if ( row > -1 )
                {
                    QList<Token*> tokens = editWidget->drags( row );
                    foreach ( Token *t, tokens )
                    {
                        if ( t == m_token )
                            continue;
                        if ( TokenWithLayout *twl = qobject_cast<TokenWithLayout*>( t ) )
                            spareWidth -= twl->width() * 100.0;
                    }
                }

                int max = qMax( spareWidth, 0.0 );

                if ( max >= m_token->width() * 100.0 )
                    m_width->setMaximum( qMax( spareWidth, 0.0 ) );
                else
                    m_width->setMaximum( m_token->width() * 100.0 );
            }
        }
        m_width->setValue( m_token->width() * 100.0 );
        if ( m_token->width() > 0.0 )
            m_fixedWidth->setChecked( true );
        else
            m_peerWidth->setChecked( true );
        m_originalWidth = m_token->width();
        m_previousWidth = m_width->value();

        m_originalAlignment = m_token->alignment();
        if ( m_originalAlignment & Qt::AlignLeft )
            m_alignLeft->setChecked(true);
        else if ( m_originalAlignment & Qt::AlignHCenter )
            m_alignCenter->setChecked(true);
        else if ( m_originalAlignment & Qt::AlignRight )
            m_alignRight->setChecked(true);

        m_bold->setChecked( m_originalBold = m_token->bold() );
        m_italic->setChecked( m_originalItalic = m_token->italic() );

        m_token->connect( m_prefix, SIGNAL( textChanged(const QString&) ), SLOT( setPrefix(const QString&) ) );
        m_token->connect( m_suffix, SIGNAL( textChanged(const QString&) ), SLOT( setSuffix(const QString&) ) );
        m_token->connect( m_width, SIGNAL( valueChanged(int) ), SLOT( setWidth(int) ) );
        m_token->connect( m_alignLeft, SIGNAL( toggled(bool) ), SLOT( setAlignLeft(bool) ) );
        m_token->connect( m_alignCenter, SIGNAL( toggled(bool) ), SLOT( setAlignCenter(bool) ) );
        m_token->connect( m_alignRight, SIGNAL( toggled(bool) ), SLOT( setAlignRight(bool) ) );
        m_token->connect( m_bold, SIGNAL( toggled(bool) ), SLOT( setBold(bool) ) );
        m_token->connect( m_italic, SIGNAL( toggled(bool) ), SLOT( setItalic(bool) ) );
    }
}

#include "LayoutEditDialog.moc"
