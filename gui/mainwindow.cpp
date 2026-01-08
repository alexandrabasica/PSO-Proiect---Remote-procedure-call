#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QSplitter>
#include <QLabel>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client("127.0.0.1", 12345)
{
    // Apply Dark Theme
    qApp->setStyle("Fusion");
    QString darkStyle = R"(
        QMainWindow { background-color: #2b2b2b; color: #ffffff; }
        QWidget { background-color: #2b2b2b; color: #ffffff; font-family: 'Segoe UI', sans-serif; font-size: 10pt; }
        QGroupBox { border: 1px solid #555; margin-top: 10px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; }
        QLineEdit, QPlainTextEdit, QTextEdit { 
            background-color: #1e1e1e; 
            color: #dcdcdc; 
            border: 1px solid #3e3e3e; 
            border-radius: 4px; 
            padding: 4px;
            selection-background-color: #264f78;
        }
        QPushButton { 
            background-color: #0e639c; 
            color: white; 
            border: none; 
            padding: 6px 12px; 
            border-radius: 4px; 
        }
        QPushButton:hover { background-color: #1177bb; }
        QPushButton:pressed { background-color: #0b4a75; }
        QPushButton:disabled { background-color: #333; color: #888; }
        QSplitter::handle { background-color: #444; }
        QLabel { color: #cccccc; }
    )";
    qApp->setStyleSheet(darkStyle);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // --- Connection Area ---
    auto *connGroup = new QGroupBox("Connection Settings", this);
    auto *connLayout = new QHBoxLayout(connGroup);
    
    hostInput = new QLineEdit("127.0.0.1", this);
    hostInput->setPlaceholderText("Host IP");
    
    portInput = new QLineEdit("12345", this);
    portInput->setPlaceholderText("Port");
    portInput->setFixedWidth(80);
    
    connectButton = new QPushButton("Connect", this);
    statusLabel = new QLabel("Not Connected", this);
    statusLabel->setStyleSheet("color: #ff6b6b; font-weight: bold;");

    connLayout->addWidget(new QLabel("Host:"));
    connLayout->addWidget(hostInput);
    connLayout->addWidget(new QLabel("Port:"));
    connLayout->addWidget(portInput);
    connLayout->addWidget(connectButton);
    connLayout->addWidget(statusLabel);
    connLayout->addStretch();

    mainLayout->addWidget(connGroup);

    // --- Splitter Area ---
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("<b>Output</b>"));
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    leftLayout->addWidget(outputArea);
    
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(new QLabel("<b>Syscall Trace</b>"));
    traceArea = new QTextEdit(this);
    traceArea->setReadOnly(true);
    rightLayout->addWidget(traceArea);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1);

    // --- Input Area ---
    auto *inputGroup = new QGroupBox("C++ Code Input", this);
    auto *inputLayout = new QVBoxLayout(inputGroup);
    
    inputEditor = new QPlainTextEdit(this);
    inputEditor->setPlaceholderText("Enter C++ code here...\nExample:\nint a=5;\nprintf(\"%d\", a);");
    inputEditor->setMinimumHeight(80);
    
    sendButton = new QPushButton("Run Code", this);
    sendButton->setEnabled(false); // Disabled until connected

    inputLayout->addWidget(inputEditor);
    inputLayout->addWidget(sendButton, 0, Qt::AlignRight);

    mainLayout->addWidget(inputGroup, 0);

    setCentralWidget(central);
    setWindowTitle("RPC Remote Execution Client");
    resize(1100, 700);

    // --- Signals ---
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
}

void MainWindow::onConnectClicked() {
    if (client.isConnected()) {
        try {
            client.close();
        } catch(...) {}
        statusLabel->setText("Disconnected");
        statusLabel->setStyleSheet("color: #ff6b6b; font-weight: bold;");
        connectButton->setText("Connect");
        hostInput->setEnabled(true);
        portInput->setEnabled(true);
        sendButton->setEnabled(false);
        outputArea->append("<i>Disconnected from server.</i>");
    } else {
        QString host = hostInput->text();
        bool ok;
        int port = portInput->text().toInt(&ok);
        if (!ok || port <= 0 || port > 65535) {
            QMessageBox::warning(this, "Validation Error", "Invalid Port Number");
            return;
        }

        try {
            // Re-create client with new host/port
            client = Client(host.toStdString(), static_cast<uint16_t>(port));
            client.connectTo();
            
            statusLabel->setText("Connected");
            statusLabel->setStyleSheet("color: #51cf66; font-weight: bold;");
            connectButton->setText("Disconnect");
            hostInput->setEnabled(false);
            portInput->setEnabled(false);
            sendButton->setEnabled(true);
            outputArea->append("<i>Connected to " + host + ":" + QString::number(port) + "! Ready to run C++.</i>");
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Connection Failed", e.what());
            statusLabel->setText("Connection Failed");
        }
    }
}

void MainWindow::onSendClicked() {
    QString message = inputEditor->toPlainText();
    if (message.trimmed().isEmpty()) return;

    outputArea->append("<b>> Sending code for execution...</b>");
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
        outputArea->append("<font color='#ff6b6b'>Error: " + QString::fromStdString(e.what()) + "</font>");
        // Check if error was due to connection loss
        if (!client.isConnected()) {
             onConnectClicked(); // Reset UI state to disconnected
        }
    }
}
