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

#include <kconfigdialog.h>

class QComboBox;
class QLineEdit;
class QCheckBox;
class Options4;

class AmarokConfigDialog : public KConfigDialog
{
    Q_OBJECT

    public:
        AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config );
        void triggerChanged();
        
    private:
        QComboBox* m_pSoundSystem;
        QComboBox* m_pSoundOutput;
        QLineEdit* m_pSoundDevice;
        QCheckBox* m_pCustomSoundDevice;
        
    protected slots:
        void updateSettings();
        void updateWidgets();
        void updateWidgetsDefault();

    private slots:
        void soundSystemChanged();
    
    protected:
        bool hasChanged();
        bool isDefault();

    private:
        Options4* m_opt4;
        bool m_changed;
};


#endif // AMAROKCONFIGDIALOG_H
