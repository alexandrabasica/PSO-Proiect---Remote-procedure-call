#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client("127.0.0.1", 12345) // adresează serverul local
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    chatArea = new QTextEdit(this);
    chatArea->setReadOnly(true);
    layout->addWidget(chatArea);

    auto *hLayout = new QHBoxLayout();
    inputLine = new QLineEdit(this);
    sendButton = new QPushButton("Trimite", this);
    hLayout->addWidget(inputLine);
    hLayout->addWidget(sendButton);

    layout->addLayout(hLayout);

    setCentralWidget(central);
    setWindowTitle("RPC GUI");

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    // încearcă să conectezi clientul
    try {
        client.connectTo();
        chatArea->append("Conectat la server!");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Eroare conexiune", e.what());
    }
}

void MainWindow::onSendClicked() {
    QString message = inputLine->text();
    if (message.isEmpty()) return;

    if (message.startsWith("trace ")) {
        chatArea->append("<b>Starting trace for: " + message.mid(6) + "</b>");
        try {
            client.trace(message.mid(6).toStdString(), [this](const std::string& line) {
                // Use QMetaObject::invokeMethod to update UI from potentially different thread if needed,
                // but here we are in the main thread loop of Qt usually, unless client blocks.
                // Since client.trace blocks, we are in the UI thread, so we can update directly.
                // However, blocking the UI thread is bad practice. For now, we accept it as per requirements "basics qt things".
                chatArea->append(QString::fromStdString(line).trimmed());
                QApplication::processEvents(); // Keep UI responsive
            });
            chatArea->append("<b>Trace finished.</b>");
        } catch (const std::exception &e) {
            chatArea->append("Eroare: " + QString::fromStdString(e.what()));
        }
    } else {
        try {
            std::string reply = client.callWithTimeout(message.toStdString(), 5);
            chatArea->append("Server: " + QString::fromStdString(reply));
        } catch (const std::exception &e) {
            chatArea->append("Eroare: " + QString::fromStdString(e.what()));
        }
    }

    inputLine->clear();
}
