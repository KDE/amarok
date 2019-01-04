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

#include "amarok_transcoding_export.h"
#include "transcoding/ui_TranscodingAssistantDialog.h"
#include "core/collections/CollectionLocationDelegate.h"
#include "core/support/Debug.h"
#include "core/transcoding/TranscodingFormat.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <KPageDialog>

class QListWidget;

namespace Transcoding
{

/**
 * A QDialog for initiating a transcoding operation.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_TRANSCODING_EXPORT AssistantDialog : public KPageDialog
{
    Q_OBJECT
public:
    /**
     * Create transcoding assistant dialog. Only encoders that encode to one of the
     * @p playableFileTypes will be enabled.
     * @param playableFileTypes the playable file types
     * @param saveSupported true if transcoding config preference can be saved
     * @param operation whether this is copying or moving
     * @param destCollectionName name of the destination collection
     * @param prevConfiguration previous configuration
     * @param parent the parent widget
     */
    AssistantDialog( const QStringList &playableFileTypes, bool saveSupported,
                     Collections::CollectionLocationDelegate::OperationType operation,
                     const QString &destCollectionName,
                     const Configuration &prevConfiguration,
                     QWidget *parent = nullptr );

    Configuration configuration() const { return m_configuration; }

    /**
     * Return true if user wants to remember this configuration per destination collection
     */
    bool shouldSave() const { return m_save; }

private:
    inline void populateFormatList();
    Configuration::TrackSelection trackSelection() const;

    Configuration m_configuration;
    bool m_save;
    QStringList m_playableFileTypes;
    Ui::AssistantDialog ui;

private Q_SLOTS:
    void onJustCopyClicked();
    void onTranscodeClicked();
    void onFormatSelect( QListWidgetItem *item );
    void onRememberToggled( bool checked );
};

} //namespace Transcoding

#endif //TRANSCODING_ASSISTANTDIALOG_H
