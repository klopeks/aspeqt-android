// 
#include "docdisplaywindow.h"
#include "ui_docdisplaywindow.h"
#include "aspeqtsettings.h"

#include <QFileDialog>
#include <QPrintDialog>
#include <QPrinter>
#include <QScroller>
#include <QLocale>
#include <QUrl>

DocDisplayWindow::DocDisplayWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DocDisplayWindow)
{
    ui->setupUi(this);
    updateManualSource();
#ifdef Q_OS_ANDROID
    QWidget *w = ui->docDisplay;
    QScroller::grabGesture(w, QScroller::TouchGesture);
#endif
}

DocDisplayWindow::~DocDisplayWindow()
{
    delete ui;
}

void DocDisplayWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        updateManualSource();
        break;
    default:
        break;
    }
}

void DocDisplayWindow::closeEvent(QCloseEvent *e)
{
    emit closed();
    e->accept();
}

void DocDisplayWindow::print(const QString &text)
{
    QTextCursor c = ui->docDisplay->textCursor();
    c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->docDisplay->setTextCursor(c);
    ui->docDisplay->insertPlainText(text);
}

void DocDisplayWindow::on_actionPrint_triggered()
{
     QPrinter printer;
     QPrintDialog *dialog = new QPrintDialog(&printer, this);
         if (dialog->exec() != QDialog::Accepted)
             return;
         ui->docDisplay->print(&printer);
}

void DocDisplayWindow::updateManualSource()
{
    QString locale = aspeqtSettings->i18nLanguage().toLower();

    if (locale == "auto") {
        locale = QLocale::system().name().toLower();
    }

    QString source = "qrc:/documentation/AspeQt User Manual-English.html";

    if (locale.startsWith("pl")) {
        source = "qrc:/documentation/AspeQt User Manual-Polish.html";
    } else if (locale.startsWith("de")) {
        source = "qrc:/documentation/AspeQt User Manual-German.html";
    }

    ui->docDisplay->setSource(QUrl(source));
}
