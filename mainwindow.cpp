#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

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
    ui->stopButton->setEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onStartButtonClicked() {
    QString directory = QFileDialog::getExistingDirectory(this, tr("Выбрать директорию"),
                                                          ui->lineEdit->text().isEmpty()
                                                              ? QDir::homePath() : ui->lineEdit->text(),
                                                          QFileDialog::ShowDirsOnly
                                                              | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        if (directory == ui->lineEdit->text()) {
            return;
        }
        if (!watcher->addPath(directory)) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось начать наблюдение за директорией: %1").arg(directory));
            return;
        }
        if (!ui->lineEdit->text().isEmpty()) {
            watcher->removePath(ui->lineEdit->text());
        }
        if (!watcher->files().isEmpty()) {
            watcher->removePaths(watcher->files());
        }
        ui->lineEdit->setText(directory);
        ui->stopButton->setEnabled(true);
        refreshWatchedFiles(directory);
        QString message = "Начато наблюдение директории: " + directory;
        logger->logChange(message.toStdString());
        ui->logTextEdit->append(message);
    }
}

void MainWindow::onStopButtonClicked() {
    watcher->removePaths(watcher->directories());
    if (!watcher->files().isEmpty()) {
        watcher->removePaths(watcher->files());
    }
    ui->lineEdit->clear();
    ui->stopButton->setEnabled(false);
    QString message = "Наблюдение директории остановлено.";
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}

void MainWindow::onDirectoryChanged(const QString &path) {
    QString message = "Директория изменилась: " + path;
    if (!watcher->directories().contains(path)) {
        message = "Наблюдение прекращено (директория недоступна): " + path;
        if (!watcher->files().isEmpty()) {
            watcher->removePaths(watcher->files());
        }
        ui->lineEdit->clear();
        ui->stopButton->setEnabled(false);
    } else {
        refreshWatchedFiles(path);
    }
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}

void MainWindow::onFileChanged(const QString &path) {
    if (QFileInfo::exists(path) && !watcher->files().contains(path)) {
        watcher->addPath(path);
    }
    QString message = "Файл изменен: " + path;
    logger->logChange(message.toStdString());
    ui->logTextEdit->append(message);
}

void MainWindow::refreshWatchedFiles(const QString &directory) {
    const QStringList current = watcher->files();
    QStringList files;
    QDir dir(directory);
    for (const QString &name : dir.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        files.append(dir.filePath(name));
    }
    for (const QString &file : current) {
        if (!files.contains(file)) {
            watcher->removePath(file);
        }
    }
    for (const QString &file : files) {
        if (!current.contains(file)) {
            watcher->addPath(file);
        }
    }
}
