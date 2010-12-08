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

#ifndef TRANSCODING_ASSISTANTDIALOG_H
#define TRANSCODING_ASSISTANTDIALOG_H

#include "src/transcoding/ui_TranscodingAssistantDialog.h"
#include "amarok_transcoding_export.h"
#include "core/transcoding/TranscodingFormat.h"
#include "core/transcoding/TranscodingConfiguration.h"
#include "core/support/Debug.h"

#include <KDialog>

#include <QListWidget>

namespace Transcoding
{

/**
 * A KDialog for initiating a transcoding operation.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_TRANSCODING_EXPORT AssistantDialog : public KDialog
{
    Q_OBJECT
public:
    AssistantDialog( QWidget *parent );

    const Configuration configuration() const { return m_configuration; }

private:
    inline void populateFormatList();
    Configuration m_configuration;
    Ui::AssistantDialog ui;

private slots:
    void onJustCopyClicked();
    void onTranscodeWithDefaultsClicked();
    void onTranscodeWithOptionsClicked();
    void onTranscodeClicked();
    void onBackClicked();
    void onCurrentChanged( int page );
    void onFormatSelect( QListWidgetItem *item );
};

} //namespace Transcoding

#endif //TRANSCODING_ASSISTANTDIALOG_H
