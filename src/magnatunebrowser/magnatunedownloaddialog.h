//
// C++ Interface: 
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

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

