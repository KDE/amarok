// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#include "amazonsearch.h"
#include "collectiondb.h"

#include <kdialog.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>


AmazonSearch::AmazonSearch( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )

{
    if ( !name ) setName( "AmazonSearch" );
    
    textLabel = new QLabel( this, "textLabel" );
    searchString = new KLineEdit( this, "searchString" );
    fileButton = new QPushButton( this, "fileButton" );
    cancelButton = new QPushButton( i18n( "Cancel" ), this, "cancelButton" );
    okButton = new QPushButton( i18n( "OK" ), this, "okButton" );
    spacer3 = new QSpacerItem( 140, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
       
    cancelButton->setAutoDefault( FALSE );
    fileButton->setAutoDefault( FALSE );
    fileButton->setPixmap( QPixmap( locate( "data", QString( "amarok/images/folder_crystal.png" ) ), "PNG" ) );
    
    AmazonSearchLayout = new QGridLayout( this, 1, 1, 11, 6, "AmazonSearchLayout"); 
    AmazonSearchLayout->setResizeMode( QLayout::Fixed );
    
    AmazonSearchLayout->addMultiCellWidget( searchString, 1, 1, 0, 3 );
    AmazonSearchLayout->addWidget( cancelButton, 2, 3 );
    AmazonSearchLayout->addWidget( okButton, 2, 2 );
    AmazonSearchLayout->addItem( spacer3, 2, 0 );
    AmazonSearchLayout->addWidget( fileButton, 2, 1 );
    AmazonSearchLayout->addMultiCellWidget( textLabel, 0, 0, 0, 3 );
    resize( QSize(363, 92).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
    
    connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( fileButton, SIGNAL( clicked() ), this, SLOT( openFile() ) );
}

AmazonSearch::~AmazonSearch()
{

}

void AmazonSearch::openFile()
{
    KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select cover image file - amaroK" ) );
    const QPixmap& pixmap( file.directory() + "/" + file.fileName() );
    
    if( !pixmap.isNull() )
    {
        emit imageReady( pixmap );
        close();
    }
}

#include "amazonsearch.moc"
