// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#include "amazonsearch.h"
#include "collectiondb.h"

#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>


AmazonSearch::AmazonSearch( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )

{
    if ( !name ) setName( "AmazonSearch" );

    m_textLabel = new QLabel( this, "textLabel" );
    m_searchString = new KLineEdit( this, "searchString" );
    QPushButton* fileButton = new QPushButton( this, "fileButton" );
    QPushButton* cancelButton = new QPushButton( i18n( "Cancel" ), this, "cancelButton" );
    QPushButton* okButton = new QPushButton( i18n( "OK" ), this, "okButton" );
    QSpacerItem* spacer3 = new QSpacerItem( 140, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    cancelButton->setAutoDefault( FALSE );
    fileButton->setAutoDefault( FALSE );
    fileButton->setPixmap( QPixmap( locate( "data", QString( "amarok/images/folder_crystal.png" ) ), "PNG" ) );

    QGridLayout* AmazonSearchLayout = new QGridLayout( this, 1, 1, 11, 6, "AmazonSearchLayout");
    AmazonSearchLayout->setResizeMode( QLayout::Fixed );
    AmazonSearchLayout->addMultiCellWidget( m_searchString, 1, 1, 0, 3 );
    AmazonSearchLayout->addWidget( cancelButton, 2, 3 );
    AmazonSearchLayout->addWidget( okButton, 2, 2 );
    AmazonSearchLayout->addItem( spacer3, 2, 0 );
    AmazonSearchLayout->addWidget( fileButton, 2, 1 );
    AmazonSearchLayout->addMultiCellWidget( m_textLabel, 0, 0, 0, 3 );

    resize( QSize(363, 92).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( fileButton, SIGNAL( clicked() ), this, SLOT( openFile() ) );
}


void AmazonSearch::openFile() //SLOT
{
    KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select cover image file - amaroK" ) );

    if ( !file.isEmpty() )
    {
        QImage image( file.directory() + "/" + file.fileName() );

        if( !image.isNull() )
        {
            emit imageReady( image );
            close();
        }
    }
}

#include "amazonsearch.moc"
