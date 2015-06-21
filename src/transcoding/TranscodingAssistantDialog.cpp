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

#include "TranscodingAssistantDialog.h"

#include "TranscodingJob.h"
#include "core/transcoding/TranscodingController.h"

#include <QIcon>
#include <KPushButton>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Transcoding;

AssistantDialog::AssistantDialog( const QStringList &playableFileTypes, bool saveSupported,
                                  Collections::CollectionLocationDelegate::OperationType operation,
                                  const QString &destCollectionName,
                                  const Configuration &prevConfiguration,
                                  QWidget *parent )
    : KPageDialog( parent, Qt::Dialog )
    , m_configuration( JUST_COPY )
    , m_save( false )
    , m_playableFileTypes( playableFileTypes )
{
    DEBUG_BLOCK
    Q_UNUSED( destCollectionName )  // keep it in signature, may be useful in future

    QWidget *uiBase = new QWidget( this );
    ui.setupUi( uiBase );
    setModal( true );
    setWindowTitle( i18n( "Transcode Tracks" ) );
    setMinimumSize( 590, 340 );
    setMaximumWidth( 590 );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(uiBase);
    okButton->setText( i18n( "Transc&ode" ) );
    okButton->setEnabled( false );
    mainLayout->addWidget(buttonBox);

    QString explanatoryText;
    QIcon justCopyIcon;
    QString justCopyText;
    QString justCopyDescription;
    switch( operation )
    {
        case Collections::CollectionLocationDelegate::Copy:
            explanatoryText = i18n(
                "While copying, you can choose to transcode your music files into another "
                "format with an encoder (codec). This can be done to save space or to "
                "make your files readable by a portable music player or a particular "
                "software program." );
            justCopyIcon = QIcon::fromTheme( "edit-copy" );
            justCopyText = i18n( "&Copy" );
            justCopyDescription = i18n( "Just copy the tracks without transcoding them." );
            break;
        case Collections::CollectionLocationDelegate::Move:
            explanatoryText = i18n(
                "While moving, you can choose to transcode your music files into another "
                "format with an encoder (codec). This can be done to save space or to "
                "make your files readable by a portable music player or a particular "
                "software program. Only successfully transcoded files will be removed "
                "from their original location." );
            justCopyIcon = QIcon::fromTheme( "go-jump" ); // Dolphin uses this icon for "move"
            justCopyText = i18n( "&Move" );
            justCopyDescription = i18n( "Just move the tracks without transcoding them." );
            break;
    }
    ui.explanatoryTextLabel->setText( explanatoryText );

    ui.justCopyButton->setIcon( justCopyIcon );
    ui.justCopyButton->setText( justCopyText );
    ui.justCopyButton->setDescription( justCopyDescription );
    ui.justCopyButton->setMinimumHeight( ui.justCopyButton->iconSize().height() + 2*10 ); //we make room for the pretty icon
    connect( ui.justCopyButton, SIGNAL(clicked()),
             this, SLOT(onJustCopyClicked()) );

    //Let's set up the codecs page...
    populateFormatList();
    connect( ui.formatListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(onFormatSelect(QListWidgetItem*)) );

    ui.formatIconLabel->hide();
    ui.formatNameLabel->hide();
    connect( buttonBox->button(QDialogButtonBox::Ok) , SIGNAL(clicked()),
             this, SLOT(onTranscodeClicked()) );

    ui.rememberCheckBox->setChecked( m_save );
    ui.rememberCheckBox->setEnabled( saveSupported );
    connect( ui.rememberCheckBox, SIGNAL(toggled(bool)),
             this, SLOT(onRememberToggled(bool)) );

    switch( prevConfiguration.trackSelection() ) //restore the previously selected TrackSelection radio button
    {
        case Configuration::TranscodeUnlessSameType:
            ui.transcodeUnlessSameTypeRadioButton->setChecked( true );
            break;
        case Configuration::TranscodeOnlyIfNeeded:
            ui.transcodeOnlyIfNeededRadioButton->setChecked( true );
            break;
        case Configuration::TranscodeAll:
            ui.transcodeAllRadioButton->setChecked( true );
    }

    ui.transcodeAllRadioButton->setEnabled( false );
    ui.transcodeUnlessSameTypeRadioButton->setEnabled( false );
    ui.transcodeOnlyIfNeededRadioButton->setEnabled( false );
}

void
AssistantDialog::populateFormatList()
{
    QSet<Encoder> available = Amarok::Components::transcodingController()->availableEncoders();

    // Add a note if no encoder is found
    ui.groupBox->setEnabled( !available.isEmpty() );
    ui.encoderNotFoundLabel->setVisible( available.isEmpty() );

    foreach( Encoder encoder, Amarok::Components::transcodingController()->allEncoders() )
    {
        if( encoder == INVALID || encoder == JUST_COPY )
            continue; // skip "fake" encoders
        Format *format = Amarok::Components::transcodingController()->format( encoder );

        QListWidgetItem *item = new QListWidgetItem( format->icon(), format->prettyName() );
        item->setToolTip( format->description() );
        item->setData( Qt::UserRole, encoder );

        // can be disabled due to unavailabilty
        bool enabled = available.contains( encoder );
        if( !enabled )
            item->setToolTip( i18nc( "Tooltip of a disabled transcoding encoder option",
                                     "Not currently available on your system." ) );

        // or because not supported by target collection
        if( enabled && !m_playableFileTypes.isEmpty() )
        {
            enabled = m_playableFileTypes.contains( format->fileExtension() );
            if( !enabled )
                item->setToolTip( i18n( "Target collection indicates this format would not be playable." ) );
        }

        Qt::ItemFlags flags = item->flags();
        if( !enabled )
            item->setFlags( flags & ~Qt::ItemIsEnabled );
        ui.formatListWidget->addItem( item );
    }
}

void
AssistantDialog::onJustCopyClicked() //SLOT
{
    QDialog::done( QDialog::Accepted );
}

void
AssistantDialog::onTranscodeClicked() //SLOT
{
    m_configuration = ui.transcodingOptionsStackedWidget->configuration( trackSelection() );
    QDialog::done( QDialog::Accepted );
}

void
AssistantDialog::onFormatSelect( QListWidgetItem *item ) //SLOT
{
    if( item )
    {
        ui.formatIconLabel->show();
        ui.formatNameLabel->show();
        Encoder encoder = static_cast< Encoder >( item->data( Qt::UserRole ).toInt() );
        const Format *format = Amarok::Components::transcodingController()->format( encoder );
        ui.formatIconLabel->setPixmap( format->icon().pixmap( 32, 32 ) );
        ui.formatNameLabel->setText( format->prettyName() );
        ui.formatIconLabel->setToolTip( format->description() );
        ui.formatIconLabel->setWhatsThis( format->description() );
        ui.formatNameLabel->setToolTip( format->description() );
        ui.formatNameLabel->setWhatsThis( format->description() );
        ui.transcodingOptionsStackedWidget->switchPage( encoder );

        ui.transcodeAllRadioButton->setEnabled( true );
        ui.transcodeUnlessSameTypeRadioButton->setEnabled( true );
        ui.transcodeOnlyIfNeededRadioButton->setEnabled( true );

        buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void
AssistantDialog::onRememberToggled( bool checked ) //SLOT
{
    m_save = checked;
}

Configuration::TrackSelection
AssistantDialog::trackSelection() const
{
    if( ui.transcodeOnlyIfNeededRadioButton->isChecked() )
        return Configuration::TranscodeOnlyIfNeeded;
    else if( ui.transcodeUnlessSameTypeRadioButton->isChecked() )
        return Configuration::TranscodeUnlessSameType;
    else
        return Configuration::TranscodeAll;
}

