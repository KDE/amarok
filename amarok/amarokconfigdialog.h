/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKCONFIGDIALOG_H
#define AMAROKCONFIGDIALOG_H

#include "osd.h"

#include <kconfigdialog.h>

class QComboBox;

class AmarokConfigDialog : public KConfigDialog
{
    Q_OBJECT

    public:
        AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config );

        // ATTRIBUTES ------
    private:
        QComboBox* m_pSoundSystem;

    private:
        bool hasChanged();
        bool isDefault();

    private slots:
        void updateSettings();
};


#endif // AMAROKCONFIGDIALOG_H
