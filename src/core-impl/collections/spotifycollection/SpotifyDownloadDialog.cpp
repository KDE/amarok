#include "SpotifyDownloadDialog.h"
#include "ui_SpotifyDownloadDialog.h"

SpotifyDownloadDialog::SpotifyDownloadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpotifyDownloadDialog)
{
    ui->setupUi(this);
}

SpotifyDownloadDialog::~SpotifyDownloadDialog()
{
    delete ui;
}
