/****************************************************************************************
 * Copyright (c) 2006 Giovanni Venturi <giovanni@kde-it.org>                            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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
 
#ifndef AMAROK_EDITFILTERDIALOG_H
#define AMAROK_EDITFILTERDIALOG_H

#include "core/meta/forward_declarations.h"
#include "widgets/MetaQueryWidget.h"
#include "widgets/TokenPool.h"

#include <QDialog>
#include <QList>

namespace Ui
{
    class EditFilterDialog;
}

class EditFilterDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit EditFilterDialog( QWidget* parent, const QString &text = QString() );
        ~EditFilterDialog() override;

        QString filter();

    Q_SIGNALS:
        void filterChanged( const QString &filter );

    private Q_SLOTS:
        void slotTokenSelected( Token *token );
        void slotTokenDestroyed( QObject *token );
        void slotAttributeChanged( const MetaQueryWidget::Filter &filter );
        void slotInvert( bool checked );
        void slotSeparatorChange();
        void slotSearchEditChanged( const QString &filterText );
        void slotReset();
        void accept() override;

        void updateAttributeEditor();
        void updateSearchEdit();

        /** Parses the given text and set's the dropTarget accordingly. */
        void updateDropTarget( const QString &filterText );

    private:
        void initTokenPool();
        Token *tokenForField( const qint64 field );

        struct Filter
        {
            MetaQueryWidget::Filter filter;
            bool inverted;
        };

        Filter &filterForToken( Token *token );

        Ui::EditFilterDialog *m_ui;
        Token *m_curToken;
        QMap< Token *, Filter > m_filters;

        QString m_separator;

        /** True if we are already updating the status.
            This blocks recursive calls to updateWidgets or parsteTextFilters. */
        bool m_isUpdating;
};

#endif /* AMAROK_EDITFILTERDIALOG_H */
