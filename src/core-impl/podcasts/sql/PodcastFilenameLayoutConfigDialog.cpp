/****************************************************************************************
 * Copyright (c) 2011 Sandeep Raghuraman <sandy.8925@gmail.com>                         *
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

#include "PodcastFilenameLayoutConfigDialog.h"
#include "ui_PodcastFilenameLayoutConfigWidget.h"

#include <QApplication>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

PodcastFilenameLayoutConfigDialog::PodcastFilenameLayoutConfigDialog( const Podcasts::SqlPodcastChannelPtr &channel, QWidget *parent )
    : KPageDialog( parent )
    , m_channel( channel )
    , m_pflc( new Ui::PodcastFilenameLayoutConfigWidget )
{
    QWidget* main = new QWidget( this );
    m_pflc->setupUi( main );

    setWindowTitle( i18nc( "Change filename layout", "Podcast Episode Filename Configuration" ) );
    setModal( true );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
//    showButtonSeparator( true ); TODO KF5: Replace with a Qt5 equivalent (if any equivalent exists)

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PodcastFilenameLayoutConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PodcastFilenameLayoutConfigDialog::reject);

    addPage(main, i18n("Settings") );
    setButtonBox(buttonBox);
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    init();
}

void
PodcastFilenameLayoutConfigDialog::init()
{
    //initialize state of the various gui items based on the channel settings

    QString filenameLayout = m_channel->filenameLayout();

    if( filenameLayout == QLatin1String( "%default%" ) )
    {
        m_pflc->m_filenameLayoutDefault->setChecked( true );
        m_pflc->m_filenameLayoutCustom->setChecked( false );
        m_choice = 0;
    }
    else
    {
        m_pflc->m_filenameLayoutDefault->setChecked( false );
        m_pflc->m_filenameLayoutCustom->setChecked( true );
        m_pflc->m_filenameLayoutText->setText( filenameLayout );
        m_choice = 1;
    }

    connect( buttonBox()->button(QDialogButtonBox::Ok) , &QAbstractButton::clicked, this, &PodcastFilenameLayoutConfigDialog::slotApply );
}


void
PodcastFilenameLayoutConfigDialog::slotApply()
{
    if( m_pflc->m_filenameLayoutCustom->isChecked() )
        m_channel->setFilenameLayout( m_pflc->m_filenameLayoutText->text() );
    else
        m_channel->setFilenameLayout( QStringLiteral("%default%") );
}

bool
PodcastFilenameLayoutConfigDialog::configure()
{
    return exec() == QDialog::Accepted;
}


