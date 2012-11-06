#ifndef SPOTIFYDOWNLOADDIALOG_H
#define SPOTIFYDOWNLOADDIALOG_H

#include <QDialog>

namespace Ui {
class SpotifyDownloadDialog;
}

class SpotifyDownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpotifyDownloadDialog(QWidget *parent = 0);
    ~SpotifyDownloadDialog();

private:
    Ui::SpotifyDownloadDialog *ui;
};

#endif // SPOTIFYDOWNLOADDIALOG_H
