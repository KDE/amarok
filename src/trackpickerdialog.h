/***************************************************************************
    begin                : Sat Sep 6 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TRACKPICKERDIALOG_H
#define TRACKPICKERDIALOG_H

#include <config.h>

#if HAVE_TUNEPIMP

#include <kdialogbase.h>

#include "ktrm.h"

class TrackPickerDialogBase;


class TrackPickerDialog : public KDialogBase
{
    Q_OBJECT

signals:
    void sigSelectionMade( KTRMResult );

public:
    TrackPickerDialog(const QString &name, const KTRMResultList &results, QWidget *parent = 0);

    KTRMResult result() const;

private slots:
    void accept();

private:
    TrackPickerDialogBase *m_base;
};

#endif // HAVE_TUNEPIMP

#endif
