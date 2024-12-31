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

static const QString filePath = QCoreApplication::applicationDirPath() + "/tasks.json";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //update greetings
    this->updateGreetings();

    connect(ui->addTaskButton, &QPushButton::clicked, this, &MainWindow::addTask);
    connect(ui->deleteTaskButton, &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(ui->saveTaskButton, &QPushButton::clicked, this, &MainWindow::saveTasks);
    connect(ui->loadTaskButton, &QPushButton::clicked, this, &MainWindow::loadTasks);

    //auto load tasks
    this->loadTasks();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addTask(){
    QString taskText = ui->taskInputLineEdit->text();
    if(!taskText.isEmpty()){
        QListWidgetItem *item = new QListWidgetItem(taskText);
        item->setCheckState(Qt::Unchecked);
        ui->taskListWidget->addItem(item);
        ui->taskInputLineEdit->clear();
    }
    else{
        QMessageBox::warning(this, "Warning", "Task cannot be empty!");
    }
}

void MainWindow::deleteTask(){
    QList<QListWidgetItem *> selectedItems = ui->taskListWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        delete ui->taskListWidget->takeItem(ui->taskListWidget->row(item));
    }
}

void MainWindow::saveTasks(){
    QJsonArray taskArray;
    for (int i = 0; i < ui->taskListWidget->count(); ++i) {
        QListWidgetItem *item = ui->taskListWidget->item(i);
        QJsonObject taskObject;
        taskObject["task"] = item->text();
        taskObject["completed"] = (item->checkState() == Qt::Checked);
        taskArray.append(taskObject);
    }

    QJsonDocument doc(taskArray);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "Success", "Tasks saved successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Could not save tasks.");
    }
}

void MainWindow::loadTasks(){
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
                ui->taskListWidget->addItem(item);
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "No saved tasks found.");
    }
}

void MainWindow::updateGreetings(){
    // Fetch the current date
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("dddd, MMMM dd, yyyy"); // Example: "Monday, January 01, 2024"

    // Determine greeting based on the time of day
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

    // Set the label text
    QString greetingText = QString("%1, today is: %2").arg(greeting, dateString);
    ui->greetingLabel->setText(greetingText);
}
