#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include "json.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void loadFile();
    void validateJson();
    void saveFile();
    void moveLine();
    void changeValueByKey();

private:
    QTextEdit *jsonEditor;
    QListWidget *errorList;
    QPushButton *loadBtn, *validateBtn, *saveBtn, *moveBtn, *changeValueByKeyBtn;
    Json jsonParser;
};

#endif