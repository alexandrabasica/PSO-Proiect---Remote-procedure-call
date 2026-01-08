#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include "Client.h"

#include <QLabel>
#include <QPlainTextEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onSendClicked();
    void onConnectClicked();

private:
    // Connection UI
    QLineEdit *hostInput;
    QLineEdit *portInput;
    QPushButton *connectButton;
    QLabel *statusLabel;

    // Main UI
    QPlainTextEdit *inputEditor;
    QTextEdit *outputArea;
    QTextEdit *traceArea;
    QPushButton *sendButton;

    Client client;
};
