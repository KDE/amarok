/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "TranscodingOptionsStackedWidget.h"

#include "core/support/Debug.h"
#include "core/transcoding/TranscodingProperty.h"
#include "core/transcoding/TranscodingController.h"

#include <QIcon>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

namespace Transcoding
{

OptionsStackedWidget::OptionsStackedWidget( QWidget *parent )
    : QStackedWidget( parent )
{
    initWelcomePage();
    for( const Encoder &encoder : Amarok::Components::transcodingController()->availableEncoders() )
    {
        Format *format = Amarok::Components::transcodingController()->format( encoder );
        m_pagesMap.insert( encoder, initCodecPage( format ) );
    }
}

void
OptionsStackedWidget::initWelcomePage()
{
    QWidget *welcomeWidget = new QWidget( this );
    QVBoxLayout *vbl = new QVBoxLayout( welcomeWidget );
    vbl->addStretch();
    QHBoxLayout *hbl = new QHBoxLayout( welcomeWidget );
    vbl->addLayout( hbl );
    hbl->addStretch();
    QLabel *arrow = new QLabel( welcomeWidget );
    arrow->setPixmap( QIcon::fromTheme( QStringLiteral("arrow-left") ).pixmap( 16, 16 ) );
    QLabel *message = new QLabel( i18n(
            "In order to configure the parameters of the transcoding operation, please "
            "pick an encoder from the list." ), this );
    message->setWordWrap( true );
    hbl->addWidget( arrow );
    hbl->addWidget( message );
    hbl->addStretch();
    vbl->addStretch();

    insertWidget( 0, welcomeWidget );
}

int
OptionsStackedWidget::initCodecPage( Format *format )
{
    m_propertyWidgetsMap.insert( format->encoder(), QList< PropertyWidget * >() );

    QWidget *codecWidget = new QWidget( this );

    QVBoxLayout *mainLayout = new QVBoxLayout( codecWidget );
    mainLayout->addStretch( 1 );

    for( Property property : format->propertyList() )
    {
        PropertyWidget *propertyWidget = PropertyWidget::create( property, codecWidget );
        mainLayout->addWidget( propertyWidget->widget() );
        m_propertyWidgetsMap[ format->encoder() ].append( propertyWidget );
        debug() << "Created config widget for " << format->prettyName()
                << ", element " << property.name();
    }

    return addWidget( codecWidget );
}

const Configuration
OptionsStackedWidget::configuration( const Configuration::TrackSelection trackSelection ) const
{
    Encoder encoder = m_pagesMap.key( currentIndex() );
    Configuration configuration = Configuration( encoder, trackSelection );

    for( PropertyWidget *propertyWidget : m_propertyWidgetsMap.value( encoder ) )
    {
        configuration.addProperty( propertyWidget->name(), propertyWidget->value() );
    }

    return configuration;
}

void
OptionsStackedWidget::switchPage( Encoder encoder)
{
    setCurrentIndex( m_pagesMap.value( encoder ) );
    Q_EMIT formatChanged( encoder );
}

} //namespace Transcoding
