#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QSplitter>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client("127.0.0.1", 12345)
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);


    auto *splitter = new QSplitter(Qt::Horizontal, this);


    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(new QLabel("<b>Code & Output</b>"));
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    leftLayout->addWidget(outputArea);
    
    
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(new QLabel("<b>Syscall Trace</b>"));
    traceArea = new QTextEdit(this);
    traceArea->setReadOnly(true);
    rightLayout->addWidget(traceArea);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);


    auto *inputLayout = new QHBoxLayout();
    inputLine = new QLineEdit(this);
    inputLine->setPlaceholderText("Enter C++ code (e.g., int a=5; printf(\"%d\", a);)");
    sendButton = new QPushButton("Run", this);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    mainLayout->addLayout(inputLayout);

    setCentralWidget(central);
    setWindowTitle("RPC C++ REPL");
    resize(1000, 600);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);

    try {
        client.connectTo();
        outputArea->append("<i>Connected to server! Ready for C++ commands.</i>");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Connection Error", e.what());
    }
}

void MainWindow::onSendClicked() {
    QString message = inputLine->text();
    if (message.isEmpty()) return;


    outputArea->append("<b>> " + message + "</b>");
    traceArea->clear(); 
    traceArea->append("<b>--- New Execution ---</b>");

    try {

        client.trace(message.toStdString(), 
            [this](const std::string& line) {
                traceArea->append(QString::fromStdString(line).trimmed());
                QApplication::processEvents();
            },
            [this](const std::string& line) {
                outputArea->append(QString::fromStdString(line).trimmed());
                QApplication::processEvents();
            }
        );
    } catch (const std::exception &e) {
        outputArea->append("<font color='red'>Error: " + QString::fromStdString(e.what()) + "</font>");
    }

    inputLine->clear();
}
