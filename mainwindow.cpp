#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qpushbutton.h"
#include "qwidget.h"
#include "qmessagebox.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QCoreApplication>
#include <QSettings>
#include <QAbstractItemModel>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QCheckBox>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString dirPath = QCoreApplication::applicationDirPath();
    QString filePath = dirPath + "/tasks.json";
    this->filePath = filePath;

    std::cout << this->filePath.toStdString() << std::endl;

    // Update greetings
    this->updateGreetings();

    // Enable drag-and-drop
    ui->taskListWidget->setDragEnabled(true);
    ui->taskListWidget->setAcceptDrops(true);
    ui->taskListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    ui->taskListWidget->setDefaultDropAction(Qt::MoveAction);

    connect(ui->deleteTaskButton, &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(ui->taskInputLineEdit, &QLineEdit::returnPressed, this, &MainWindow::addTask);
    connect(ui->taskListWidget->model(), &QAbstractItemModel::rowsMoved, this, &MainWindow::saveTasks);

    // Theme setup
    connect(ui->themeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::changeTheme);

    // Reminder setup
    connect(ui->reminderDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::updateReminderTime);
    connect(ui->enableReminderCheckBox, &QCheckBox::toggled, this, &MainWindow::onReminderCheckBoxToggled);
    connect(ui->taskInputLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateInputState);

    QSettings settings("MyApp", "TodoApp");
    QString theme = settings.value("theme", "Light").toString();
    ui->themeComboBox->setCurrentText(theme);
    applyTheme(theme);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/tray_icon.png"));
    trayIcon->setToolTip("To-Do App");
    trayIcon->show();
    reminderTimer = new QTimer(this);
    connect(reminderTimer, &QTimer::timeout, this, &MainWindow::checkReminders);
    reminderTimer->start(60000); // Check every minute

    // Initial UI state
    ui->reminderDateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600)); // Default to 1 hour from now
    onReminderCheckBoxToggled(ui->enableReminderCheckBox->isChecked());
    updateInputState();

    // Auto load tasks
    this->loadTasks();
}

MainWindow::~MainWindow()
{
    delete trayIcon;
    delete reminderTimer;
    delete ui;
}

void MainWindow::addTask()
{
    QString taskText = ui->taskInputLineEdit->text();
    if (!taskText.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem(taskText);
        item->setCheckState(Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // Ensure checkable
        if (ui->enableReminderCheckBox->isChecked()) {
            QDateTime reminderTime = ui->reminderDateTimeEdit->dateTime();
            if (reminderTime > QDateTime::currentDateTime()) {
                item->setData(Qt::UserRole, reminderTime);
            } else {
                QMessageBox::warning(this, "Warning", "Reminder time must be in the future!");
                return;
            }
        }
        ui->taskListWidget->addItem(item);
        ui->taskInputLineEdit->clear();
        ui->enableReminderCheckBox->setChecked(false);
        this->saveTasks();
    } else {
        QMessageBox::warning(this, "Warning", "Task cannot be empty!");
    }
}

void MainWindow::deleteTask()
{
    bool deleted = false;
    for (int i = ui->taskListWidget->count() - 1; i >= 0; --i) {
        QListWidgetItem *item = ui->taskListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            delete ui->taskListWidget->takeItem(i);
            deleted = true;
        }
    }
    if (!deleted) {
        QMessageBox::information(this, "Info", "No tasks selected for deletion.");
    }
    this->saveTasks();
}

void MainWindow::saveTasks()
{
    QJsonArray taskArray;
    for (int i = 0; i < ui->taskListWidget->count(); ++i) {
        QListWidgetItem *item = ui->taskListWidget->item(i);
        QJsonObject taskObject;
        taskObject["task"] = item->text();
        taskObject["completed"] = (item->checkState() == Qt::Checked);
        QVariant reminderVar = item->data(Qt::UserRole);
        if (reminderVar.isValid()) {
            QDateTime reminderTime = reminderVar.toDateTime();
            taskObject["reminder"] = reminderTime.toString(Qt::ISODate);
        }
        taskArray.append(taskObject);
    }

    QJsonDocument doc(taskArray);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    } else {
        QMessageBox::warning(this, "Error", "Could not save tasks.");
    }
}

void MainWindow::loadTasks()
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            ui->taskListWidget->clear();
            QJsonArray taskArray = doc.array();
            for (const QJsonValue &value : taskArray) {
                QJsonObject taskObject = value.toObject();
                QString taskText = taskObject["task"].toString();
                bool completed = taskObject["completed"].toBool();

                QListWidgetItem *item = new QListWidgetItem(taskText);
                item->setCheckState(completed ? Qt::Checked : Qt::Unchecked);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // Ensure checkable
                if (taskObject.contains("reminder")) {
                    QDateTime reminderTime = QDateTime::fromString(taskObject["reminder"].toString(), Qt::ISODate);
                    if (reminderTime.isValid()) {
                        item->setData(Qt::UserRole, reminderTime);
                    }
                }
                ui->taskListWidget->addItem(item);
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "No saved tasks found.");
    }
}

void MainWindow::updateGreetings()
{
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("dddd, MMMM dd, yyyy");
    QTime currentTime = QTime::currentTime();
    QString greeting;
    int hour = currentTime.hour();
    if (hour < 12) {
        greeting = "Good morning";
    } else if (hour < 18) {
        greeting = "Good afternoon";
    } else {
        greeting = "Good evening";
    }
    QString greetingText = QString("%1, today is: %2").arg(greeting, dateString);
    ui->greetingLabel->setText(greetingText);
}

void MainWindow::changeTheme(const QString &themeName)
{
    applyTheme(themeName);
    QSettings settings("MyApp", "TodoApp");
    settings.setValue("theme", themeName);
}

void MainWindow::applyTheme(const QString &themeName)
{
    QString stylesheet;
    if (themeName == "Dark") {
        stylesheet = R"(
            QWidget {
                background-color: #2E2E2E;
                color: #FFFFFF;
            }
            QLineEdit, QListWidget, QComboBox, QDateTimeEdit {
                background-color: #3C3C3C;
                color: #FFFFFF;
                border: 1px solid #555555;
            }
            QPushButton, QCheckBox {
                background-color: #555555;
                color: #FFFFFF;
                border: 1px solid #777777;
            }
            QPushButton:hover {
                background-color: #666666;
            }
        )";
    } else {
        stylesheet = R"(
            QWidget {
                background-color: #FFFFFF;
                color: #000000;
            }
            QLineEdit, QListWidget, QComboBox, QDateTimeEdit {
                background-color: #FFFFFF;
                color: #000000;
                border: 1px solid #CCCCCC;
            }
            QPushButton, QCheckBox {
                background-color: #F0F0F0;
                color: #000000;
                border: 1px solid #999999;
            }
            QPushButton:hover {
                background-color: #E0E0E0;
            }
        )";
    }
    setStyleSheet(stylesheet);
}

void MainWindow::checkReminders()
{
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < ui->taskListWidget->count(); ++i) {
        QListWidgetItem *item = ui->taskListWidget->item(i);
        QVariant reminderVar = item->data(Qt::UserRole);
        if (reminderVar.isValid()) {
            QDateTime reminderTime = reminderVar.toDateTime();
            if (reminderTime <= now) {
                trayIcon->showMessage("Task Reminder",
                                      QString("Task due: %1").arg(item->text()),
                                      QSystemTrayIcon::Information,
                                      5000);
                item->setData(Qt::UserRole, QVariant());
                saveTasks();
            }
        }
    }
}

void MainWindow::updateReminderTime(const QDateTime &dateTime)
{
    if (ui->enableReminderCheckBox->isChecked() && dateTime <= QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "Warning", "Reminder time must be in the future!");
        ui->reminderDateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    }
    updateInputState();
}

void MainWindow::onReminderCheckBoxToggled(bool checked)
{
    ui->reminderDateTimeEdit->setEnabled(checked);
    updateInputState();
}

void MainWindow::updateInputState()
{
    bool isReminderValid = !ui->enableReminderCheckBox->isChecked() ||
                           (ui->reminderDateTimeEdit->dateTime() > QDateTime::currentDateTime());
    // Only disable taskInputLineEdit if reminder is invalid and checkbox is checked
    ui->taskInputLineEdit->setEnabled(isReminderValid);
}
