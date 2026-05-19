#include "cassettedialog.h"
#include "ui_cassettedialog.h"

#include <QTimer>
#include <QtDebug>
#include <QScreen>
#include <QMovie>
#include <QFileInfo>
#include <QFontMetrics>
#include <QResizeEvent>

QMovie *movie = NULL;

CassetteDialog::CassetteDialog(QWidget *parent, const QString &fileName, CassettePlaybackEngine engine)
    : QDialog(parent), ui(new Ui::CassetteDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);
    setWindowState(Qt::WindowFullScreen);

    mFileName = fileName;
    mEngine = engine;
    mTapeFileLabel = nullptr;

    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    movie = new QMovie(":images/tape.gif");
    ui->tape_label->setMovie(movie);
    ui->tape_label->setAlignment(Qt::AlignCenter);
    mTapeFileLabel = new QLabel(ui->tape_label);
    mTapeFileLabel->setAlignment(Qt::AlignCenter);
    mTapeFileLabel->setStyleSheet("color: rgb(0, 0, 0); background: transparent;");
    mTapeFileLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    mTapeFileLabel->setWordWrap(false);
    movie->start();
    movie->setPaused(true);
    connect(movie, &QMovie::frameChanged, this, [this](int) { updateTapeFileOverlay(); });
    updateTapeFileOverlay();
#endif
    ui->progressBar->setVisible(true);
    worker = new CassetteWorker;
    mTotalDuration = worker->mTotalDuration;
    mRemainingTime = mTotalDuration;
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("AspeQt is ready to playback the cassette image file '%1'.\n\n"
                          "On your Atari, hold OPTION + START while powering on (or RESET) "
                          "to boot from cassette. Alternatively, type \"CLOAD\" in BASIC.\n\n"
                          "Atari will BEEP — at that moment press the RETURN key on the "
                          "Atari AND click OK below at the same time.\n\n"
                          "(RETURN is what tells Atari to start receiving tape data — "
                          "without it the data is sent but ignored.)")
                       .arg(mFileName)
                       .arg(minutes)
                       .arg(seconds, 2, 10, QChar('0')));

    connect(worker, SIGNAL(statusChanged(int)), this, SLOT(progress(int)), Qt::QueuedConnection);
    connect(worker, SIGNAL(finished()), this, SLOT(reject()));
}

CassetteDialog::~CassetteDialog()
{
    disconnect(this, SLOT(reject()));
    if (worker->isRunning()) {
        worker->setPriority(QThread::NormalPriority);
    }
    worker->wait();
    delete worker;
    delete ui;
}

void CassetteDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        updateTapeFileOverlay();
        break;
    default:
        break;
    }
}

void CassetteDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateTapeFileOverlay();
}

int CassetteDialog::exec()
{
    if (!worker->loadCasImage(mFileName, mEngine)) {
        return 0;
    }

    return QDialog::exec();
}

void CassetteDialog::accept()
{
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("Playing back cassette image.\n\n"
                          "Estimated time left: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));

    worker->start(QThread::TimeCriticalPriority);
    mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(tick()));
    mTimer->start(1000);
    if (movie) {
        movie->setPaused(false);
    }
}

void CassetteDialog::progress(int remainingTime)
{
    mTotalDuration = worker->mTotalDuration;
    mRemainingTime = remainingTime;
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("Playing back cassette image.\n\n"
                          "Estimated time left: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
    ui->progressBar->setMaximum(mTotalDuration);
    ui->progressBar->setValue(mTotalDuration - mRemainingTime);
}

void CassetteDialog::tick()
{
    if (mRemainingTime < 1000) {
        mRemainingTime = 1000;
    }
    emit progress(mRemainingTime - 1000);
}

void CassetteDialog::updateTapeFileOverlay()
{
#ifdef Q_OS_ANDROID
    if (!mTapeFileLabel || !ui->tape_label) {
        return;
    }

    const QSize referenceSize(240, 146);
    const QRect referenceTextRect(16, 14, 206, 26);

    QSize gifSize = referenceSize;
    if (movie) {
        if (!movie->currentPixmap().isNull()) {
            gifSize = movie->currentPixmap().size();
        } else if (movie->frameRect().isValid()) {
            gifSize = movie->frameRect().size();
        }
    }

    const QRect labelRect = ui->tape_label->contentsRect();
    const int gifX = labelRect.x() + (labelRect.width() - gifSize.width()) / 2;
    const int gifY = labelRect.y() + (labelRect.height() - gifSize.height()) / 2;
    const qreal scaleX = static_cast<qreal>(gifSize.width()) / referenceSize.width();
    const qreal scaleY = static_cast<qreal>(gifSize.height()) / referenceSize.height();

    QRect targetRect(gifX + qRound(referenceTextRect.x() * scaleX),
                     gifY + qRound(referenceTextRect.y() * scaleY),
                     qRound(referenceTextRect.width() * scaleX),
                     qRound(referenceTextRect.height() * scaleY));

    targetRect = targetRect.intersected(ui->tape_label->rect());
    mTapeFileLabel->setGeometry(targetRect);

    QFont font = mTapeFileLabel->font();
    font.setBold(true);
    font.setPixelSize(qMax(10, qRound(16 * qMin(scaleX, scaleY))));
    mTapeFileLabel->setFont(font);

    QString baseName = QFileInfo(mFileName).completeBaseName();
    if (baseName.isEmpty()) {
        baseName = QFileInfo(mFileName).fileName();
    }

    QFontMetrics metrics(font);
    QString displayText = metrics.elidedText(baseName, Qt::ElideRight, qMax(0, targetRect.width() - 4));
    mTapeFileLabel->setText(displayText);
    mTapeFileLabel->setToolTip(baseName);
    mTapeFileLabel->raise();
    mTapeFileLabel->show();
#endif
}
