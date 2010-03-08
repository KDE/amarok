/***************************************************************************
                              likebackdialog.h
                             -------------------
    begin                : unknown
    imported to LB svn   : 3 june, 2009
    copyright            : (C) 2006 by Sebastien Laout
                           (C) 2008-2009 by Valerio Pilo, Sjors Gielen
    email                : sjors@kmess.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIKEBACKDIALOG_H
#define LIKEBACKDIALOG_H

#include <KDialog>

#include <QButtonGroup>

#include "likeback.h"

#include "ui_likebackdialog.h"


class LikeBackDialog : public KDialog, private Ui::LikeBackDialog
{
  Q_OBJECT
  public:
    // Constructor
                  LikeBackDialog( LikeBack::Button reason, const QString &initialComment, const QString &windowPath,
                                  const QString &context, LikeBack *likeBack );
    // Destructor
                ~LikeBackDialog();

  private:
    // Construct the introductory text of the dialog
    QString introductionText();

  private:
    // Additional referred window information
    QString       m_context;
    // The parent LikeBack instance
    LikeBack     *m_likeBack;
    // Group of comment type checkboxes
    QButtonGroup *m_typeGroup_;
    // The id of the window this dialog refers to
    QString       m_windowPath;
    // Identifier of the sent request
    int           m_requestNumber_;

  private slots:
    // Check if the UI should allow the user to send the comment
    void verify();
    // Send the comment to the developers site (reimpl. from KDialog)
    void slotButtonClicked( int button );
    // Display confirmation of the sending action
    void requestFinished( int id, bool error );
};

#endif
