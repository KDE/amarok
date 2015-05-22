/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#define DEBUG_PREFIX "PhotosApplet"

#include "PhotosApplet.h"
#include "PhotosScrollWidget.h"

// Amarok
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "context/ContextView.h"
#include "context/engines/photos/PhotosInfo.h"
#include "context/widgets/AppletHeader.h"

// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>

PhotosApplet::PhotosApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_settingsIcon( 0 )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
}

void
PhotosApplet::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    // Create label
    enableHeader( true );
    setHeaderText( i18n( "Photos" ) );

    // Set the collapse size
    setCollapseHeight( m_header->height() );
    setCollapseOffHeight( 220 );
    setMaximumHeight( 220 );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    // Icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addRightHeaderAction( settingsAction );
    connect( m_settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    m_widget = new PhotosScrollWidget( this );
    m_widget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    connect( m_widget, SIGNAL(photoAdded()), SLOT(photoAdded()) );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( m_header );
    layout->addItem( m_widget );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Photos Applet");
    m_nbPhotos = config.readEntry( "NbPhotos", "10" ).toInt();
    m_Animation = config.readEntry( "Animation", "Fading" );
    m_KeyWords = config.readEntry( "KeyWords", QStringList() );

    if( m_Animation == i18nc( "animation type", "Automatic" ) )
        m_widget->setMode( 0 );
    else if( m_Animation == i18n( "Interactive" ) )
        m_widget->setMode( 1 );
    else // fading
        m_widget->setMode( 2 );

    Plasma::DataEngine *engine = dataEngine( "amarok-photos" );
    engine->setProperty( "fetchSize", m_nbPhotos );
    engine->setProperty( "keywords", m_KeyWords );
    engine->connectSource( "photos", this );
}

PhotosApplet::~PhotosApplet()
{
    DEBUG_BLOCK
}

void
PhotosApplet::stopped()
{
    DEBUG_BLOCK
    setHeaderText( i18n( "Photos: No Track Playing" ) );
    m_widget->clear();
    m_widget->hide();
    setBusy( false );
    setMinimumHeight( m_header->height() );
    setCollapseHeight( m_header->height() );
    setCollapseOn();
    updateConstraints();
}

void
PhotosApplet::photoAdded()
{
    setBusy( false );
    setHeaderText( i18ncp( "@title:window Number of photos of artist",
                           "1 Photo: %2",
                           "%1 Photos: %2",
                           m_widget->count(),
                           m_currentArtist ) );
}

void
PhotosApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    if( name != QLatin1String("photos") || data.isEmpty() )
        return;

    QString text;

    if( data.contains( "message" ) )
    {
        text = data["message"].toString();
        if( text.contains( QLatin1String("Fetching") ) )
        {
            debug() << "received message: Fetching";
            setHeaderText( i18n( "Photos: %1", text ) );
            setMinimumHeight( m_header->height() );
            setCollapseHeight( m_header->height() );
            setCollapseOn();
            m_widget->clear();
            m_widget->hide();
            if( canAnimate() )
                setBusy( true );
        }
        else if( text.contains( QLatin1String("stopped") ) )
        {
            debug() << "received message: stopped";
            stopped();
        }
        else
        {
            debug() << "received message:" << text;
            setHeaderText( i18n( "Photos: %1", text ) );
            m_widget->hide();
            setMinimumHeight( m_header->height() );
            setCollapseHeight( m_header->height() );
            setCollapseOn();
            setBusy( false );
        }
    }
    else if( data.contains( "data" ) )
    {
        m_widget->clear();
        m_currentArtist = text = data["artist"].toString();
        PhotosInfo::List photos = data["data"].value< PhotosInfo::List >();
        debug() << "received data for:" << text << photos.count();
        setHeaderText( i18n( "Photos: %1", text ) );
        if( photos.isEmpty() )
        {
            setBusy( false );
            setMinimumHeight( m_header->height() );
            setCollapseHeight( m_header->height() );
            setCollapseOn();
            return;
        }
        setBusy( true );
        m_widget->setPhotosInfoList( photos );
        setMinimumHeight( 220 );
        setCollapseOff();
        m_widget->show();
        layout()->invalidate();
    }
    else
    {
        setMinimumHeight( m_header->height() );
        setCollapseHeight( m_header->height() );
        setCollapseOn();
        m_widget->clear();
        m_widget->hide();
        setBusy( false );
    }
    updateConstraints();
}

void
PhotosApplet::createConfigurationInterface( KConfigDialog *parent )
{
    parent->setButtons( KDialog::Ok | KDialog::Cancel );

    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Photos Settings" ), "preferences-system");

    ui_Settings.animationComboBox->setCurrentIndex( ui_Settings.animationComboBox->findText( m_Animation ) );
    ui_Settings.photosSpinBox->setValue( m_nbPhotos );
    ui_Settings.additionalkeywordsLineEdit->setText( m_KeyWords.join(", ") );
    connect( parent, SIGNAL(accepted()), this, SLOT(saveSettings()) );
}

void
PhotosApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Photos Applet");

    m_nbPhotos = ui_Settings.photosSpinBox->value();
    m_Animation = ui_Settings.animationComboBox->currentText();
    m_KeyWords = ui_Settings.additionalkeywordsLineEdit->text().split(", ");
    config.writeEntry( "NbPhotos", m_nbPhotos );
    config.writeEntry( "Animation", m_Animation );
    config.writeEntry( "KeyWords", m_KeyWords );

    m_widget->setMode( ui_Settings.animationComboBox->currentIndex() );
    m_widget->clear();

    Plasma::DataEngine *engine = dataEngine( "amarok-photos" );
    engine->setProperty( "fetchSize", m_nbPhotos );
    engine->setProperty( "keywords", m_KeyWords );
    engine->query( QLatin1String( "photos:forceUpdate" ) );
}

