//
// C++ Implementation: 
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution


#include "magnatunedownloaddialog.h"
#include <qcombobox.h>
#include <qtextedit.h>
#include <qcheckbox.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
 #include "debug.h"

MagnatuneDownloadDialog::MagnatuneDownloadDialog(QWidget* parent, const char* name, bool modal, WFlags fl)
: MagnatuneDownloadDialogBase(parent,name, modal,fl)
{
   downloadTargetURLRequester->fileDialog()->setMode(KFile::Directory);

}

MagnatuneDownloadDialog::~MagnatuneDownloadDialog()
{
}

void MagnatuneDownloadDialog::setMessage( QString message )
{
   infoEdit->setText(message);
}

void MagnatuneDownloadDialog::addFormat( QString name, QString link )
{

   m_map[name] = link; 
   formatComboBox->insertItem(name);

}

void MagnatuneDownloadDialog::downloadButtonClicked( )
{

  QString sourceUrl = m_map[formatComboBox->currentText()];
  QString targetUrl = downloadTargetURLRequester->url();

  emit(downloadAlbum(sourceUrl, targetUrl));

  close();

}

/*$SPECIALIZATION$*/


#include "magnatunedownloaddialog.moc"

