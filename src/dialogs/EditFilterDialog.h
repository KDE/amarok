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

#include "core/meta/Meta.h"
#include "widgets/MetaQueryWidget.h"
#include "widgets/TokenPool.h"

#include <KDialog>
#include <QList>

namespace Ui
{
    class EditFilterDialog;
}

class TokenDropTarget;

class EditFilterDialog : public KDialog
{
    Q_OBJECT

    public:
        explicit EditFilterDialog( QWidget* parent, const QString &text = QString() );
        ~EditFilterDialog();

        QString filter() const;

    signals:
        void filterChanged( const QString &filter );

    private slots:
        void slotTokenSelected( QWidget *token );
        void slotTokenDropTargetChanged();
        void slotAttributeChanged( const MetaQueryWidget::Filter &filter );
        void slotInvert( bool checked );
        void slotSeparatorChange( const QString &separator );
        void slotReset();
        void accept();

    private:
        void initTokenPool();
        Token *tokenForField( const qint64 field );
        void parseTextFilter( const QString &text );
        void updateMetaQueryWidgetView();

        struct Filter
        {
            MetaQueryWidget::Filter filter;
            bool inverted;
        };

        Ui::EditFilterDialog *m_ui;
        TokenDropTarget *m_dropTarget;
        Token *m_curToken;
        QMap< Token *, Filter > m_filters;

        QString m_separator;
};

#endif /* AMAROK_EDITFILTERDIALOG_H */
