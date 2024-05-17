#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>  // Убедитесь, что этот заголовок включен

Logger::Logger(const std::string& logFilePath) {
    logFile.open(logFilePath, std::ios_base::app);
}

void Logger::logChange(const std::string& changeDescription) {
    if (logFile.is_open()) {
        logFile << changeDescription << std::endl;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , logger(std::make_shared<Logger>("changes.log"))
    , watcher(std::make_unique<QFileSystemWatcher>(this))
{
    ui->setupUi(this);
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(watcher.get(), &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDirectoryChanged);
    connect(watcher.get(), &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onStartButtonClicked() {
    QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                          "",
                                                          QFileDialog::ShowDirsOnly
                                                              | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        watcher->addPath(directory);
        QString message = "Начато наблюдение директории: " + directory;
        logger->logChange(message.toStdString());
        ui->logTextEdit->append(message);
    }
}

void MainWindow::onStopButtonClicked() {
    watcher->removePaths(watcher->directories());
    QString message = "Наблюдение директории остановлено.";
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}

void MainWindow::onDirectoryChanged(const QString &path) {
    QString message = "Директория изменилась: " + path;
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}

void MainWindow::onFileChanged(const QString &path) {
    QString message = "Файл изменен: " + path;
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}
