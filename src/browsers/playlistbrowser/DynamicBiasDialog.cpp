/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010,2011 Ralf Engels <ralf-engels@gmx.de>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicBiasDialog.h"

#include "core/support/Debug.h"
#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>

PlaylistBrowserNS::BiasDialog::BiasDialog( const Dynamic::BiasPtr &bias, QWidget* parent )
    : QDialog( parent )
    , m_mainLayout( nullptr )
    , m_biasLayout( nullptr )
    , m_descriptionLabel( nullptr )
    , m_biasWidget( nullptr )
    , m_origBias( bias )
    , m_bias( bias->clone() ) // m_bias is a clone
{
    setWindowTitle( i18nc( "Bias dialog window title", "Edit bias" ) );
    m_mainLayout = new QVBoxLayout( this );


    // -- the bias selection combo
    QLabel* selectionLabel = new QLabel( i18nc("Bias selection label in bias view.", "Match Type:" ) );
    m_biasSelection = new QComboBox();
    QHBoxLayout *selectionLayout = new QHBoxLayout();
    selectionLabel->setBuddy( m_biasSelection );
    selectionLayout->addWidget( selectionLabel );
    selectionLayout->addWidget( m_biasSelection );
    selectionLayout->addStretch( 1 );
    m_mainLayout->addLayout( selectionLayout );

    // -- bias itself
    m_descriptionLabel = new QLabel( QLatin1String("") );
    m_descriptionLabel->setWordWrap( true );
    m_mainLayout->addWidget( m_descriptionLabel );

    m_biasLayout = new QVBoxLayout();
    m_mainLayout->addLayout( m_biasLayout );

    // -- button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    m_mainLayout->addWidget( buttonBox );

    factoriesChanged();
    biasReplaced( Dynamic::BiasPtr(), m_bias );

    connect( Dynamic::BiasFactory::instance(), &Dynamic::BiasFactory::changed,
             this, &PlaylistBrowserNS::BiasDialog::factoriesChanged );
    connect( m_biasSelection, QOverload<int>::of(&QComboBox::activated),
             this, &PlaylistBrowserNS::BiasDialog::selectionChanged );
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &PlaylistBrowserNS::BiasDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &PlaylistBrowserNS::BiasDialog::reject);
}

void PlaylistBrowserNS::BiasDialog::accept()
{
    // use the newly edited bias
    m_origBias->replace( m_bias ); // tell the old bias it has just been replaced
    QDialog::accept();
}

void PlaylistBrowserNS::BiasDialog::reject()
{
    // do nothing.
    QDialog::reject();
}

PlaylistBrowserNS::BiasDialog::~BiasDialog()
{ }

void
PlaylistBrowserNS::BiasDialog::factoriesChanged()
{
    m_biasSelection->clear();

    disconnect( Dynamic::BiasFactory::instance(), &Dynamic::BiasFactory::changed,
                this, &PlaylistBrowserNS::BiasDialog::factoriesChanged );

    // -- add all the bias types to the list
    bool factoryFound = false;
    QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
    for( int i = 0; i <  factories.count(); i++ )
    {
        Dynamic::AbstractBiasFactory* factory = factories.at( i );
        m_biasSelection->addItem( factory->i18nName(), QVariant( factory->name() ) );

        // -- set the current index if we have found our own factory
        if( m_bias && factory->name() == m_bias->name() )
        {
            factoryFound = true;
            m_biasSelection->setCurrentIndex( i );
            m_descriptionLabel->setText( factory->i18nDescription() );
        }
    }

    // -- In cases of replacement bias
    if( !factoryFound )
    {
        m_biasSelection->addItem( m_bias->name() );
        m_biasSelection->setCurrentIndex( m_biasSelection->count() - 1 );
        m_descriptionLabel->setText( i18n( "This bias is a replacement for another bias\n"
                                         "which is currently not loaded or deactivated.\n"
                                         "The original bias name was %1.", m_bias->name() ) );
    }

    connect( Dynamic::BiasFactory::instance(), &Dynamic::BiasFactory::changed,
             this, &PlaylistBrowserNS::BiasDialog::factoriesChanged );
}

void
PlaylistBrowserNS::BiasDialog::selectionChanged( int index )
{
    DEBUG_BLOCK;
    Q_ASSERT( m_biasSelection );

    QString biasName = m_biasSelection->itemData( index ).toString();

    Dynamic::BiasPtr oldBias( m_bias );
    Dynamic::BiasPtr newBias( Dynamic::BiasFactory::fromName( biasName ) );
    if( !newBias )
    {
        warning() << "Could not create bias with name:"<<biasName;
        return;
    }

    debug() << "replace bias" << oldBias->toString() << "with" << newBias->toString();
    m_bias->replace( newBias ); // tell the old bias it has just been replaced
    debug() << "replaced";

    // -- if the new bias is AndBias, try to add the old biase(s) into it
    Dynamic::AndBias *oldABias = qobject_cast<Dynamic::AndBias*>(oldBias.data());
    Dynamic::AndBias *newABias = qobject_cast<Dynamic::AndBias*>(newBias.data());
    if( newABias ) {
        if( oldABias ) {
            for( int i = 0; i < oldABias->biases().count(); i++ )
            {
                newABias->appendBias( oldABias->biases()[i] );
            }
        }
        else
        {
            newABias->appendBias( oldBias );
        }
    }
}

void
PlaylistBrowserNS::BiasDialog::biasReplaced( const Dynamic::BiasPtr &oldBias, Dynamic::BiasPtr newBias )
{
    Q_UNUSED( oldBias );

    if( m_biasWidget )
    {
        m_biasLayout->removeWidget( m_biasWidget );
        m_biasWidget->deleteLater();
        m_biasWidget = nullptr;
    }

    m_bias = newBias;
    if( !newBias )
        return;

    connect( newBias.data(), &Dynamic::AbstractBias::replaced,
             this, &PlaylistBrowserNS::BiasDialog::biasReplaced );

    m_biasWidget = newBias->widget( nullptr );
    if( !m_biasWidget )
        m_biasWidget = new QLabel( i18n("This bias has no settings.") );
    m_biasLayout->addWidget( m_biasWidget );

    factoriesChanged(); // update the bias description and select the new combo entry
}



