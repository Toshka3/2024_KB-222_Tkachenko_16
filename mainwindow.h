#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <memory>
#include <fstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Logger {
public:
    Logger(const std::string& logFilePath);
    void logChange(const std::string& changeDescription);
private:
    std::ofstream logFile;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);

private:
    Ui::MainWindow *ui;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<QFileSystemWatcher> watcher;
};

#endif // MAINWINDOW_H
