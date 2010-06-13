/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#include "SearchWidget.h"
#include "EditFilterDialog.h"

#include <QToolBar>
#include <QVBoxLayout>

#include <KIcon>
#include <KLocale>
#include <KLineEdit>
#include <KHBox>
#include <KPushButton>


SearchWidget::SearchWidget( QWidget *parent, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterAction( 0 )
{
    init( parent, advanced );
}

SearchWidget::SearchWidget( QWidget *parent, QWidget *caller, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterAction( 0 )
{
    init( parent, advanced );
    setup( caller );
}

void
SearchWidget::setup( QObject* caller )
{
    connect( m_sw, SIGNAL( editTextChanged( const QString & ) ), caller,
             SLOT( slotSetFilterTimeout() ) );
    connect( this, SIGNAL( filterNow() ), caller,
             SLOT( slotFilterNow() ) );
    connect( m_sw, SIGNAL( returnPressed() ), caller, SLOT( slotFilterNow() ) );
    connect( m_sw, SIGNAL( downPressed() ), caller, SLOT( setFocus() ) );
}

///Private
void
SearchWidget::init( QWidget *parent, bool advanced )
{
    Q_UNUSED( parent )
    setContentsMargins( 0, 0, 0, 0 );
    KHBox *searchBox = new KHBox( this );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

    m_sw = new Amarok::ComboBox( searchBox );
    m_sw->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_sw->setFrame( true );
    m_sw->setCompletionMode( KGlobalSettings::CompletionPopup );
    m_sw->completionObject()->setIgnoreCase( true );
    m_sw->setToolTip( i18n( "Enter space-separated terms to search." ) );
    m_sw->addItem( KStandardGuiItem::find().icon(), QString() );
    connect( m_sw, SIGNAL(returnPressed(const QString&)), SLOT(addCompletion(const QString&)) );
    connect( m_sw, SIGNAL(activated(int)), SLOT(onComboItemActivated(int)));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( searchBox );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );
    setClickMessage( i18n( "Enter search terms here" ) );

    m_toolBar = new QToolBar( searchBox );
    m_toolBar->setFixedHeight( m_sw->sizeHint().height() );

    if ( advanced ) {
        m_filterAction = new QAction( KIcon( "document-properties" ), i18n( "Edit filter" ), this );
        m_filterAction->setObjectName( "filter" );
        m_toolBar->addAction( m_filterAction );

        connect ( m_filterAction, SIGNAL( triggered() ), this, SLOT( slotShowFilterEditor() ) );
    }
}

void
SearchWidget::setSearchString( const QString &searchString )
{
    m_sw->setEditText( searchString );
    emit filterNow();
}

void
SearchWidget::addCompletion( const QString &text )
{
    int index = m_sw->findText( text );
    if( index == -1 )
    {
        m_sw->addItem( KStandardGuiItem::find().icon(), text );
        m_sw->completionObject()->addItem( text );
    }
    else
    {
        m_sw->setCurrentIndex( index );
    }
}

void
SearchWidget::onComboItemActivated( int index )
{
    // if data of UserRole exists, use that as the actual filter string
    const QString userFilter = m_sw->itemData( index ).toString();
    if( userFilter.isEmpty() )
        m_sw->setEditText( m_sw->itemText(index) );
    else
        m_sw->setEditText( userFilter );
}

void
SearchWidget::slotShowFilterEditor()
{
    EditFilterDialog *fd = new EditFilterDialog( this, m_sw->currentText() );

    connect( fd, SIGNAL(filterChanged(const QString&) ), m_sw, SLOT(setEditText(const QString&)) );

    fd->exec();
}

QToolBar * SearchWidget::toolBar()
{
    return m_toolBar;
}

void SearchWidget::showAdvancedButton(bool show)
{
    if ( show ) {
        if ( m_filterAction != 0 ) {
            m_filterAction = new QAction( KIcon( "document-properties" ), i18n( "Edit filter" ), this );
            m_filterAction->setObjectName( "filter" );
            m_toolBar->addAction( m_filterAction );
            connect ( m_filterAction, SIGNAL( triggered() ), this, SLOT( slotShowFilterEditor() ) );
        }
    }
    else
    {
        delete m_filterAction;
        m_filterAction = 0;
    }
}

void SearchWidget::setClickMessage( const QString &message )
{
    KLineEdit *edit = qobject_cast<KLineEdit*>( m_sw->lineEdit() );
    edit->setClickMessage( message );
}

#include "SearchWidget.moc"
