/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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
#include "core/support/Debug.h"
#include "dialogs/EditFilterDialog.h"
#include "widgets/BoxWidget.h"

#include <QAction>
#include <QIcon>
#include <QLineEdit>
#include <QStandardPaths>
#include <QToolBar>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KStandardGuiItem>


SearchWidget::SearchWidget( QWidget *parent, bool advanced )
    : QWidget( parent )
    , m_sw( nullptr )
    , m_filterAction( nullptr )
    , m_timeout( 500 )
    , m_runningSearches( 0 )
{
    setContentsMargins( 0, 0, 0, 0 );
    BoxWidget *searchBox = new BoxWidget( false );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

    m_sw = new Amarok::ComboBox( searchBox );
    m_sw->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_sw->setFrame( true );
    m_sw->setCompletionMode( KCompletion::CompletionPopup );
    m_sw->completionObject()->setIgnoreCase( true );
    m_sw->setToolTip( i18n( "Enter space-separated terms to search." ) );
    m_sw->addItem( KStandardGuiItem::find().icon(), QString() );
    connect( m_sw, QOverload<int>::of(&QComboBox::activated),
             this, &SearchWidget::onComboItemActivated );
    connect( m_sw, &Amarok::ComboBox::editTextChanged, this, &SearchWidget::resetFilterTimeout );
    connect( m_sw, QOverload<const QString&>::of(&KComboBox::returnPressed),
             this, &SearchWidget::filterNow ); // filterNow() calls addCompletion()
    connect( m_sw, QOverload<const QString&>::of(&KComboBox::returnPressed),
            this, &SearchWidget::returnPressed );
    connect( m_sw, &Amarok::ComboBox::downPressed, this, &SearchWidget::advanceFocus );

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( searchBox );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );
    setClickMessage( i18n( "Enter search terms here" ) );

    m_toolBar = new QToolBar( searchBox );
    m_toolBar->setFixedHeight( m_sw->sizeHint().height() );

    if( advanced )
    {
        m_filterAction = new QAction( QIcon::fromTheme( QStringLiteral("document-properties") ), i18n( "Edit filter" ), this );
        m_filterAction->setObjectName( QStringLiteral("filter") );
        m_toolBar->addAction( m_filterAction );

        connect( m_filterAction, &QAction::triggered, this, &SearchWidget::slotShowFilterEditor );
    }

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, &QTimer::timeout, this, &SearchWidget::filterNow );

    m_animationTimer.setInterval( 500 );
    connect( &m_animationTimer, &QTimer::timeout, this, &SearchWidget::nextAnimationTick );
}

void
SearchWidget::resetFilterTimeout()
{
    m_filterTimer.start( m_timeout );
}

void
SearchWidget::filterNow()
{
    m_filterTimer.stop();
    addCompletion( m_sw->currentText() );
    Q_EMIT filterChanged( m_sw->currentText() );
}

void
SearchWidget::advanceFocus()
{
    focusNextChild();
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

    index = m_sw->findText( text );
    m_sw->setCurrentIndex( index );
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
    fd->setAttribute( Qt::WA_DeleteOnClose );
    m_filterAction->setEnabled( false );

    connect( fd, &EditFilterDialog::filterChanged, m_sw, &Amarok::ComboBox::setEditText );
    connect( fd, &QDialog::finished, this, &SearchWidget::slotFilterEditorFinished );

    fd->show();
}

void
SearchWidget::slotFilterEditorFinished( int result )
{
    m_filterAction->setEnabled( true );

    if( result && !m_sw->currentText().isEmpty() ) // result == QDialog::Accepted
        addCompletion( m_sw->currentText() );
}

QToolBar *
SearchWidget::toolBar()
{
    return m_toolBar;
}

void
SearchWidget::showAdvancedButton( bool show )
{
    if( show )
    {
        if( m_filterAction != nullptr )
        {
            m_filterAction = new QAction( QIcon::fromTheme( QStringLiteral("document-properties") ), i18n( "Edit filter" ), this );
            m_filterAction->setObjectName( QStringLiteral("filter") );
            m_toolBar->addAction( m_filterAction );
            connect( m_filterAction, &QAction::triggered, this, &SearchWidget::slotShowFilterEditor );
        }
    }
    else
    {
        delete m_filterAction;
        m_filterAction = nullptr;
    }
}

void
SearchWidget::setClickMessage( const QString &message )
{
    QLineEdit *edit = qobject_cast<QLineEdit*>( m_sw->lineEdit() );
    edit->setPlaceholderText( message );
}

void
SearchWidget::setTimeout( quint16 newTimeout )
{
    m_timeout = newTimeout;
}

// public slots:

void
SearchWidget::setSearchString( const QString &searchString )
{
    if( searchString != currentText() ) {
        m_sw->setEditText( searchString );
        filterNow();
    }
}

void
SearchWidget::searchStarted()
{
    m_runningSearches++;

    // start the animation
    if( !m_animationTimer.isActive() )
    {
        m_sw->setItemIcon( m_sw->currentIndex(), QIcon( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/loading1.png") ) ) );
        m_currentFrame = 0;
        m_animationTimer.start();
    }

    // If another search is running it might still have a part of the animation set as its icon.
    // As the currentIndex() has changed we don't know which one. We now have to iterate through
    // all of them and set the icon correctly. It's not as bad as it sounds: the number is quite
    // limited.

    for( int i = 0; i < m_sw->count(); i++ )
    {
        if( i != m_sw->currentIndex() ) // not the current one, which should be animated!
            m_sw->setItemIcon( i, KStandardGuiItem::find().icon() );
    }
}

void
SearchWidget::searchEnded()
{
    if( m_runningSearches > 0 ) // just to be sure...
        m_runningSearches--;

    // stop the animation
    if( m_runningSearches == 0 )
    {
        m_animationTimer.stop();
        saveLineEditStatus();
        m_sw->setItemIcon( m_sw->currentIndex(), KStandardGuiItem::find().icon() );
        restoreLineEditStatus();
    }
}


// private Q_SLOTS:

void
SearchWidget::nextAnimationTick()
{
    saveLineEditStatus();

    // switch frames
    if( m_currentFrame == 0 )
        m_sw->setItemIcon( m_sw->currentIndex(), QIcon( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/loading2.png") ) ) );
    else
        m_sw->setItemIcon( m_sw->currentIndex(), QIcon( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/loading1.png") ) ) );

    restoreLineEditStatus();
    m_currentFrame = !m_currentFrame;
}


// private:

void
SearchWidget::restoreLineEditStatus()
{
    // restore text changes made by the user
    m_sw->setEditText( m_text );

    if( m_hasSelectedText )
        m_sw->lineEdit()->setSelection( m_selectionStart, m_selectionLength ); // also sets cursor
    else
        m_sw->lineEdit()->setCursorPosition( m_cursorPosition );
}

void
SearchWidget::saveLineEditStatus()
{
    // save text changes made by the user
    m_text = m_sw->lineEdit()->text();
    m_cursorPosition = m_sw->cursorPosition();
    m_hasSelectedText = m_sw->lineEdit()->hasSelectedText();
    m_selectionStart = m_sw->lineEdit()->selectionStart();
    m_selectionLength = m_sw->lineEdit()->selectedText().length();
}

