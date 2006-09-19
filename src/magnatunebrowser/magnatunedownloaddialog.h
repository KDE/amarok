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

#ifndef MAGNATUNEDOWNLOADDIALOG_H
#define MAGNATUNEDOWNLOADDIALOG_H

#include "magnatunedownloaddialogbase.h"

#include <qmap.h>

class MagnatuneDownloadDialog : public MagnatuneDownloadDialogBase
{
  Q_OBJECT

public:
  MagnatuneDownloadDialog(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
  ~MagnatuneDownloadDialog();
  /*$PUBLIC_FUNCTIONS$*/

  void setMessage(QString message);
  void addFormat(QString name, QString link);

signals:

   void downloadAlbum(QString url, QString downloadLocation);

public slots:
  /*$PUBLIC_SLOTS$*/

protected:
  /*$PROTECTED_FUNCTIONS$*/
   typedef QMap<QString, QString> DownloadUrlMap; 
   DownloadUrlMap m_map;

protected slots:
  /*$PROTECTED_SLOTS$*/
  void downloadButtonClicked();

};

#endif

