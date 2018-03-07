/****************************************************************************************
 * Copyright (c) 2012 Edward "hades" Toroshchin <amarok@hades.name>                     *
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

#include "MagnatuneNeedUpdateWidget.h"
#include "ui_MagnatuneNeedUpdateWidget.h"

#include "MagnatuneConfig.h"

#include <core/support/Debug.h>

MagnatuneNeedUpdateWidget::MagnatuneNeedUpdateWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::MagnatuneNeedUpdateWidget)
{
    ui->setupUi(this);

    connect(ui->update, &QPushButton::clicked, this, &MagnatuneNeedUpdateWidget::startUpdate );
    connect(ui->autoUpdate, &QCheckBox::stateChanged, this, &MagnatuneNeedUpdateWidget::saveSettings );

    ui->autoUpdate->setCheckState( MagnatuneConfig().autoUpdateDatabase()?
                                   Qt::Checked : Qt::Unchecked );
}

void
MagnatuneNeedUpdateWidget::enable()
{
    ui->update->setEnabled(true);
}

void
MagnatuneNeedUpdateWidget::disable()
{
    ui->update->setEnabled(false);
}

void
MagnatuneNeedUpdateWidget::startUpdate()
{
    disable();
    emit wantUpdate();
}

void
MagnatuneNeedUpdateWidget::saveSettings()
{
    DEBUG_BLOCK

    MagnatuneConfig config;
    config.setAutoUpdateDatabase( ui->autoUpdate->checkState() == Qt::Checked );
    config.save();
}
