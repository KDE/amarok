/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#include "debug.h"
#include "magnatunedownloaddialog.h"

#include <kfiledialog.h>
#include <kurlrequester.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qtextedit.h>


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

