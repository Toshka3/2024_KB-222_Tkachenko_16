#include "../mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileSystemWatcher>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

class MainWindowTest : public QObject {
    Q_OBJECT

private:
    static void chooseDirectory(const QString &path) {
        QTimer::singleShot(0, qApp, [path]() {
            auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->selectFile(path);
            static_cast<QDialog *>(dialog)->accept();
        });
    }

    static void cancelDirectory() {
        QTimer::singleShot(0, qApp, []() {
            auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->reject();
        });
    }

private slots:
    void watchSwitchCancelAndStop() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QDir base(temporary.path());
        QVERIFY(base.mkdir("first"));
        QVERIFY(base.mkdir("second"));
        const QString first = base.filePath("first");
        const QString second = base.filePath("second");

        MainWindow window;
        window.show();
        auto *start = window.findChild<QPushButton *>("startButton");
        auto *stop = window.findChild<QPushButton *>("stopButton");
        auto *path = window.findChild<QLineEdit *>("lineEdit");
        auto *log = window.findChild<QTextEdit *>("logTextEdit");
        auto *watcher = window.findChild<QFileSystemWatcher *>();
        QVERIFY(start && stop && path && log && watcher);
        QVERIFY(!stop->isEnabled());
        QVERIFY(path->isReadOnly());
        QVERIFY(log->isReadOnly());
        QVERIFY(path->text().isEmpty());

        chooseDirectory(first);
        start->click();
        QCOMPARE(watcher->directories(), QStringList{first});
        QCOMPARE(path->text(), first);
        QVERIFY(stop->isEnabled());

        cancelDirectory();
        start->click();
        QCOMPARE(watcher->directories(), QStringList{first});

        chooseDirectory(second);
        start->click();
        QCOMPARE(watcher->directories(), QStringList{second});
        QCOMPARE(path->text(), second);

        stop->click();
        QVERIFY(watcher->directories().isEmpty());
        QVERIFY(path->text().isEmpty());
        QVERIFY(!stop->isEnabled());
        QCOMPARE(log->toPlainText().count("Начато наблюдение"), 2);
        QCOMPARE(log->toPlainText().count("остановлено"), 1);
    }

    void resizingKeepsControlsVisible() {
        MainWindow window;
        window.resize(480, 320);
        window.show();
        const auto *central = window.centralWidget();
        for (const auto *name : {"startButton", "stopButton", "lineEdit", "logTextEdit"}) {
            const auto *widget = window.findChild<QWidget *>(name);
            QVERIFY(widget);
            QVERIFY(central->rect().contains(widget->geometry().topLeft()));
            QVERIFY(central->rect().contains(widget->geometry().bottomRight()));
        }
        QVERIFY(window.findChild<QTextEdit *>("logTextEdit")->height() > 100);
        if (qEnvironmentVariableIsSet("UI_SCREENSHOT_PATH")) {
            QVERIFY(window.grab().save(qgetenv("UI_SCREENSHOT_PATH")));
        }
    }

    void fileChangesAppearInLog() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString existing = temporary.filePath("existing.txt");
        const QString added = temporary.filePath("added.txt");
        {
            QFile file(existing);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        MainWindow window;
        window.show();
        auto *watcher = window.findChild<QFileSystemWatcher *>();
        auto *log = window.findChild<QTextEdit *>("logTextEdit");
        chooseDirectory(temporary.path());
        window.findChild<QPushButton *>("startButton")->click();
        QVERIFY(watcher->files().contains(existing));

        {
            QFile file(existing);
            QVERIFY(file.open(QIODevice::Append));
            file.write("updated");
        }
        QTRY_VERIFY_WITH_TIMEOUT(log->toPlainText().contains("Файл изменен: " + existing), 3000);

        {
            QFile file(added);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        QTRY_VERIFY_WITH_TIMEOUT(watcher->files().contains(added), 3000);
        {
            QFile file(added);
            QVERIFY(file.open(QIODevice::Append));
            file.write("updated");
        }
        QTRY_VERIFY_WITH_TIMEOUT(log->toPlainText().contains("Файл изменен: " + added), 3000);

        window.findChild<QPushButton *>("stopButton")->click();
        QVERIFY(watcher->files().isEmpty());
    }

    void directoryRemovalReturnsToIdle() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QDir base(temporary.path());
        QVERIFY(base.mkdir("watched"));
        MainWindow window;
        window.show();
        auto *stop = window.findChild<QPushButton *>("stopButton");
        auto *path = window.findChild<QLineEdit *>("lineEdit");
        auto *log = window.findChild<QTextEdit *>("logTextEdit");
        chooseDirectory(base.filePath("watched"));
        window.findChild<QPushButton *>("startButton")->click();
        QVERIFY(stop->isEnabled());
        QVERIFY(base.rmdir("watched"));
        QTRY_VERIFY_WITH_TIMEOUT(!stop->isEnabled(), 3000);
        QVERIFY(path->text().isEmpty());
        QVERIFY(log->toPlainText().contains("директория недоступна"));
    }
};

QTEST_MAIN(MainWindowTest)
#include "test_mainwindow.moc"
