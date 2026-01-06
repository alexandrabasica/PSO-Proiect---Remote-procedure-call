#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include "Client.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onSendClicked();

private:
    QLineEdit *inputLine;
    QTextEdit *outputArea;
    QTextEdit *traceArea;
    QPushButton *sendButton;

    Client client;
};
