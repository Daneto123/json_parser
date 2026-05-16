#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto *centralWidget = new QWidget;
    auto *mainLayout = new QVBoxLayout(centralWidget);
    
    jsonEditor = new QTextEdit("{\n  \"name\": \"test\"\n}");
    mainLayout->addWidget(jsonEditor);
    
    // Buttons
    auto *btnLayout = new QHBoxLayout;
    loadBtn = new QPushButton("Load File");
    validateBtn = new QPushButton("Validate");
    saveBtn = new QPushButton("Save File");
    moveBtn = new QPushButton("Move Line");
    changeValueByKeyBtn = new QPushButton("Change Value By Key");
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadFile);
    connect(validateBtn, &QPushButton::clicked, this, &MainWindow::validateJson);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveFile);
    connect(moveBtn, &QPushButton::clicked, this, &MainWindow::moveLine);
    connect(changeValueByKeyBtn, &QPushButton::clicked, this, &MainWindow::changeValueByKey);
    
    btnLayout->addWidget(loadBtn);
    btnLayout->addWidget(validateBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(moveBtn);
    btnLayout->addWidget(changeValueByKeyBtn);
    mainLayout->addLayout(btnLayout);
    
    errorList = new QListWidget;
    mainLayout->addWidget(errorList);
    
    setCentralWidget(centralWidget);
    resize(800, 600);
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open JSON file");
    if (fileName.isEmpty()) return;

    std::string content = jsonParser.readFile(fileName.toStdString());

    jsonEditor->setPlainText(QString::fromStdString(content));
}

void MainWindow::validateJson() {
    errorList->clear();
    
    bool isValid = jsonParser.validate(jsonEditor->toPlainText().toStdString());
    errorList->addItem(isValid ? "JSON is valid" : "Wrong syntax!");
}

void MainWindow::saveFile() {
    errorList->clear();

    QString fileName = QFileDialog::getSaveFileName(this, "Save JSON file");

    if (fileName.isEmpty()){
        return;
    }

    std::string path = fileName.toStdString();

    jsonParser.save_to_file(path);

    errorList->addItem("Saved successfully");
}

void MainWindow::moveLine() {
    errorList->clear();

    bool ok1, ok2;

    QString from = QInputDialog::getText(this, "Move Line", "From:",QLineEdit::Normal ,"", &ok1);
    if (!ok1) return;

    QString to = QInputDialog::getText(this, "Move Line", "To:", QLineEdit::Normal, "", &ok2);
    if (!ok2) return;

    std::string fromStr = from.toStdString();
    std::string toStr = to.toStdString();
    std::string json = jsonEditor->toPlainText().toStdString();

    //jsonParser.move(from, to);

    jsonEditor->setPlainText(QString::fromStdString(json));
}

void MainWindow::changeValueByKey() {
    errorList->clear();

    bool ok1, ok2;

    QString key = QInputDialog::getText(this, "Change Value", "Key:", QLineEdit::Normal, "", &ok1);
    if (!ok1 || key.isEmpty()) return;

    QString value = QInputDialog::getText(this, "Change Value", "New value:", QLineEdit::Normal, "", &ok2);
    if (!ok2) return;

    std::string keyStr = key.toStdString();
    std::string valueStr = value.toStdString();

    bool found = jsonParser.set(keyStr, valueStr);

    if (found){
        errorList->addItem("Value updated successfully");
    }else{
        errorList->addItem("Key not found");
    }
   
}