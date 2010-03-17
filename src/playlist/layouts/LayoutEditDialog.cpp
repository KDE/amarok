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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
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

#include <QtDebug>

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
    l4->addWidget( m_fixedWidth = new QRadioButton( i18n( "Custom" ), this ) );
    m_fixedWidth->setToolTip( i18n( "Either a fixed (absolute) value, or a relative value (e.g. 128px or 12%)." ) );
    m_fixedWidth->setChecked( true );
#if HAVE_WIDTH_MODES
    l4->addWidget( m_fitContent = new QRadioButton( i18n( "Fit content" ), this ) );
    m_fitContent->setToolTip( i18n( "Fit the element text" ) );
#endif
    l4->addWidget( m_peerWidth = new QRadioButton( i18n( "Automatic" ), this ) );
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
    l3->addWidget( m_underline = new QToolButton( this ) );
    l3->addStretch();
    l1->addLayout( l3 );

    QDialogButtonBox *box = new QDialogButtonBox(this);
    box->addButton( QDialogButtonBox::Cancel );
    box->addButton( QDialogButtonBox::Ok );
    connect( box, SIGNAL( rejected() ), SLOT( close() ) );
    connect( box, SIGNAL( accepted() ), SLOT( apply() ) );
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
    m_underline->setIcon( KIcon( "format-text-underline" ) );
    m_underline->setCheckable( true );

}

void LayoutEditDialog::apply()
{
    if ( !m_token )
        return;

    m_token->setPrefix( m_prefix->text() );
    m_token->setSuffix( m_suffix->text() );
    m_token->setWidth( m_width->value() );
    if ( m_alignLeft->isChecked() )
        m_token->setAlignment( Qt::AlignLeft );
    else if ( m_alignCenter->isChecked() )
        m_token->setAlignment( Qt::AlignHCenter );
    else if ( m_alignRight->isChecked() )
        m_token->setAlignment( Qt::AlignRight );
    m_token->setBold( m_bold->isChecked() );
    m_token->setItalic( m_italic->isChecked() );
    m_token->setUnderline( m_underline->isChecked() );

    // we do this here to avoid reliance on the connection order (i.e. prevent close before apply)
    if ( sender() )
        close();
}

void LayoutEditDialog::close()
{
    m_token = 0;
    QDialog::close();
}

void LayoutEditDialog::setPeerWidth( bool peer )
{
    if ( peer )
    {
        m_previousWidth = m_width->value();
        m_width->setRange( 0, 100 );
        m_width->setValue( 0 );
    }
    else
    {
        m_width->setValue( m_previousWidth );
        m_width->setRange( 1, 100 );
    }
}


void LayoutEditDialog::setToken( TokenWithLayout *t )
{
    setWindowTitle( i18n( "Configuration for '%1'", t->name() ) );
    
    apply();
    m_token = t;
    if ( m_token )
    {
        m_element->setText( m_token->name() );
        m_prefix->setText( m_token->prefix() );
        m_suffix->setText( m_token->suffix() );


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
                    foreach ( Token *token, tokens )
                    {
                        if ( token == m_token )
                            continue;
                        if ( TokenWithLayout *twl = qobject_cast<TokenWithLayout*>( token ) )
                            spareWidth -= twl->width() * 100.0;
                    }
                }

                int max = qMax( spareWidth, qreal( 0.0 ) );

                if ( max >= m_token->width() * 100.0 )
                    m_width->setMaximum( qMax( spareWidth, qreal( 0.0 ) ) );
                else
                    m_width->setMaximum( m_token->width() * 100.0 );
            }
        }
        m_width->setValue( m_token->width() * 100.0 );
        m_previousWidth = m_width->value();
        
        if ( m_token->width() > 0.0 )
            m_fixedWidth->setChecked( true );
        else
            m_peerWidth->setChecked( true );

        if ( m_token->alignment() & Qt::AlignLeft )
            m_alignLeft->setChecked(true);
        else if ( m_token->alignment() & Qt::AlignHCenter )
            m_alignCenter->setChecked(true);
        else if ( m_token->alignment() & Qt::AlignRight )
            m_alignRight->setChecked(true);

        m_bold->setChecked( m_token->bold() );
        m_italic->setChecked( m_token->italic() );
        m_underline->setChecked( m_token->underline() );
    }
}

#include "LayoutEditDialog.moc"
