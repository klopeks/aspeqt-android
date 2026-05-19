#include "autobootdialog.h"
#include "ui_autobootdialog.h"
#include "mainwindow.h"

#include <QTime>
#include <QDebug>
#include <QScreen>
#include <QDialogButtonBox>

extern QString g_exefileName;

AutoBootDialog::AutoBootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoBootDialog)
{    
    setWindowState(Qt::WindowFullScreen);
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
    reload = false;
    open = false;
    ui->progressBar->setVisible(true);
}

AutoBootDialog::~AutoBootDialog()
{
    delete ui;
}

void AutoBootDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void AutoBootDialog::closeEvent(QCloseEvent *)
{
    if (!reload && !open) g_exefileName = "";
}

bool AutoBootDialog::reloadRequested() const
{
    return reload;
}

bool AutoBootDialog::openRequested() const
{
    return open;
}

void AutoBootDialog::booterStarted()
{
    ui->label->setText(tr("Atari is loading the booter."));
}

void AutoBootDialog::booterLoaded()
{
    ui->label->setText(tr("Atari is loading the program.\n\nFor some programs you may have to close this dialog manually when the program starts."));
}

void AutoBootDialog::blockRead(int current, int all)
{
    ui->progressBar->setMaximum(all);
    ui->progressBar->setValue(current);
}

void AutoBootDialog::loaderDone()
{
    ui->label->setText(tr("Program loaded.\n\nUse Retry to start the same executable again, Open to choose another executable, or Cancel to return to the main screen."));
    if (ui->progressBar->maximum() > 0) {
        ui->progressBar->setValue(ui->progressBar->maximum());
    }
}
void AutoBootDialog::onClick(QAbstractButton *button)
{
    switch (ui->buttonBox->standardButton(button)) {
    case QDialogButtonBox::Retry:
        reloadExe();
        break;
    case QDialogButtonBox::Open:
        openExe();
        break;
    case QDialogButtonBox::Cancel:
        g_exefileName="";
        close();
        break;
    default:
        break;
    }
}

void AutoBootDialog::reloadExe()
{
    reload = true;
    close();
}

void AutoBootDialog::openExe()
{
    open = true;
    close();
}
