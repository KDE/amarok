/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TRANSCODEDIALOG_H
#define TRANSCODEDIALOG_H

#include "ui_TranscodeDialog.h"
#include "core/transcoding/TranscodeFormat.h"
#include "core/support/Debug.h"

#include <KDialog>

/**
 * A KDialog for initiating a transcode operation.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_EXPORT TranscodeDialog : public KDialog
{
    Q_OBJECT
public:
    TranscodeDialog( /*const KUrl::List &urlList,*/ QWidget *parent );

    TranscodeFormat transcodeFormat() const;

private:
    TranscodeFormat m_format;
    Ui::TranscodeDialog ui;
    //KUrl::List m_urlList;
private slots:
    void onJustCopyClicked();
    void onTranscodeWithDefaultsClicked();
    void onTranscodeWithOptionsClicked();
};

#endif // TRANSCODEDIALOG_H
