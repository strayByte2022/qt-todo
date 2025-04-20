#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QSystemTrayIcon>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateGreetings();
    void addTask();
    void deleteTask();
    void saveTasks();
    void loadTasks();
    void changeTheme(const QString &themeName);
    void checkReminders();
    void updateReminderTime(const QDateTime &dateTime);
    void onReminderCheckBoxToggled(bool checked);
    void updateInputState();

private:
    void applyTheme(const QString &themeName);
    Ui::MainWindow *ui;
    QString filePath;
    QSystemTrayIcon *trayIcon;
    QTimer *reminderTimer;
};
#endif // MAINWINDOW_H
