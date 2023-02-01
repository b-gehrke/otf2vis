#include "MainWindow.hpp"

#include <QErrorMessage>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <utility>

#include "src/models/AppSettings.hpp"
#include "src/ui/widgets/License.hpp"
#include "src/ui/widgets/Help.hpp"
#include "src/ui/widgets/TimeInputField.hpp"
#include "src/ui/widgets/Timeline.hpp"
#include "src/ui/TimeUnit.hpp"
#include "src/ui/windows/FilterPopup.hpp"
#include "src/ui/widgets/TraceOverviewDock.hpp"


MainWindow::MainWindow(QString filepath) : QMainWindow(nullptr), filepath(std::move(filepath)) {
    if (this->filepath.isEmpty()) {
        this->promptFile();
    }
    this->loadSettings();
    this->loadTrace();

    this->createMenus();
    this->createToolBars();
    this->createDockWidgets();
    this->createCentralWidget();
}

MainWindow::~MainWindow() {
    delete data;
    delete callbacks;
    delete reader;
}

void MainWindow::createMenus() {
    auto menuBar = this->menuBar();

    /// File menu
    auto openTraceAction = new QAction(tr("&Open..."), this);
    openTraceAction->setShortcut(tr("Ctrl+O"));
    connect(openTraceAction, &QAction::triggered, this, &MainWindow::openNewTrace);
    auto openRecentMenu = new QMenu(tr("&Open recent"));
    if (AppSettings::getInstance().recentlyOpenedFiles().isEmpty()) {
        auto emptyAction = openRecentMenu->addAction(tr("&(Empty)"));
        emptyAction->setEnabled(false);
    } else {
        // TODO this is not updated on call to clear
        for (const auto &recent: AppSettings::getInstance().recentlyOpenedFiles()) {
            auto recentAction = new QAction(recent, openRecentMenu);
            openRecentMenu->addAction(recentAction);
            connect(recentAction, &QAction::triggered, [&,this] {
                // TODO memory is not valid anymore on lambda call
               this->setFilepath(recent);
               this->loadTrace();
            });
        }
        openRecentMenu->addSeparator();

        auto clearRecentMenuAction = new QAction(tr("&Clear history"));
        openRecentMenu->addAction(clearRecentMenuAction);
        connect(clearRecentMenuAction, &QAction::triggered, [&] {
            AppSettings::getInstance().recentlyOpenedFilesClear();
            openRecentMenu->clear();
        });
    }

    auto quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    auto fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(openTraceAction);
    fileMenu->addMenu(openRecentMenu);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    /// View Menu
    auto filterAction = new QAction(tr("&Filter"));
    filterAction->setShortcut(tr("Ctrl+S"));
    connect(filterAction, SIGNAL(triggered()), this, SLOT(openFilterPopup()));

    auto searchAction = new QAction(tr("&Find"));
    searchAction->setShortcut(tr("Ctrl+F"));
    connect(searchAction, SIGNAL(triggered()), this, SLOT(openFilterPopup()));

    auto resetZoomAction = new QAction(tr("&Reset zoom"));
    connect(resetZoomAction, SIGNAL(triggered()), this, SLOT(resetZoom()));
    resetZoomAction->setShortcut(tr("Ctrl+R"));

    auto viewMenu = menuBar->addMenu(tr("&View"));
    viewMenu->addAction(filterAction);
    viewMenu->addAction(searchAction);
    viewMenu->addAction(resetZoomAction);

    /// Window menu
    auto minimizeAction = new QAction(tr("&Minimize"), this);
    minimizeAction->setShortcut(tr("Ctrl+M"));
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

    auto windowMenu = menuBar->addMenu(tr("&Window"));
    windowMenu->addAction(minimizeAction);

    /// Help menu
    auto aboutAction = new QAction(tr("&View license"), this);
    connect(aboutAction, &QAction::triggered, this, [] {
        auto license = new License;
        license->show();
    });
    auto showHelpAction = new QAction(tr("&Show help"), this);
    showHelpAction->setShortcut(tr("F1"));
    connect(showHelpAction, &QAction::triggered, this, [] {
        auto help = new Help;
        help->show();
    });

    auto helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(showHelpAction);
}

void MainWindow::createToolBars() {
    // Top toolbar contains preview/control of whole trace
    this->topToolbar = new QToolBar(this);
    this->topToolbar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, this->topToolbar);

    // Bottom toolbar contains control fields
    this->bottomToolbar = new QToolBar(this);
    this->bottomToolbar->setMovable(false);
    addToolBar(Qt::BottomToolBarArea, this->bottomToolbar);

    auto bottomContainerWidget = new QWidget(this->bottomToolbar);
    auto containerLayout = new QHBoxLayout(bottomContainerWidget);
    bottomContainerWidget->setLayout(containerLayout);

    // TODO populate with initial time stamps
    this->startTimeInputField = new TimeInputField("Start", TimeUnit::Second, types::TraceTime(0),
                                                   bottomContainerWidget);
    this->startTimeInputField->setUpdateFunction(
        [this](auto newStartTime) { this->data->setSelectionBegin(newStartTime); });
    containerLayout->addWidget(this->startTimeInputField);
    this->endTimeInputField = new TimeInputField("End", TimeUnit::Second, types::TraceTime(0), bottomContainerWidget);
    this->endTimeInputField->setUpdateFunction([this](auto newEndTime) { this->data->setSelectionEnd(newEndTime); });
    containerLayout->addWidget(this->endTimeInputField);

    connect(data, SIGNAL(beginChanged(types::TraceTime)), this->startTimeInputField, SLOT(setTime(types::TraceTime)));
    connect(data, SIGNAL(endChanged(types::TraceTime)), this->endTimeInputField, SLOT(setTime(types::TraceTime)));

    this->bottomToolbar->addWidget(bottomContainerWidget);

}

void MainWindow::createDockWidgets() {
    this->slotInformation = new SlotInformationDock();
    // @formatter:off
    connect(data, SIGNAL(slotSelected(Slot*)), slotInformation, SLOT(setSlot(Slot*)));
    connect(slotInformation, SIGNAL(zoomToWindow(types::TraceTime,types::TraceTime)), data, SLOT(setSelection(types::TraceTime,types::TraceTime)));
    // @formatter:on
    this->addDockWidget(Qt::RightDockWidgetArea, this->slotInformation);

    this->traceOverview = new TraceOverviewDock(this->data);
    this->addDockWidget(Qt::TopDockWidgetArea, this->traceOverview);
}

void MainWindow::createCentralWidget() {
    auto timeline = new Timeline(data, this);
    this->setCentralWidget(timeline);
}

void MainWindow::setFilepath(QString newFilepath) {
    this->filepath = std::move(newFilepath);
}

void MainWindow::promptFile() {
    auto newFilePath = QFileDialog::getOpenFileName(this, QFileDialog::tr("Open trace"), QString(),
                                                    QFileDialog::tr("OTF Traces (*.otf *.otf2)"));

    // TODO this is still not really a great way to deal with that, especially for the initial open
    if (newFilePath.isEmpty()) {
        auto errorMsg = new QErrorMessage(nullptr);
        errorMsg->showMessage("The chosen file is invalid!");
    } else {
        this->filepath = newFilePath;
    }
}

void MainWindow::loadTrace() {
    reader = new otf2::reader::reader(this->filepath.toStdString());
    callbacks = new ReaderCallbacks(*reader);

    reader->set_callback(*callbacks);
    reader->read_definitions();
    reader->read_events();

    auto slots = callbacks->getSlots();
    auto communications = callbacks->getCommunications();
    auto collectives = callbacks->getCollectiveCommunications();
    auto trace = new FileTrace(slots, communications, collectives, callbacks->duration());

    data = new TraceDataProxy(trace, settings, this);
}

void MainWindow::loadSettings() {
    settings = new ViewSettings();
}

void MainWindow::resetZoom() {
    data->setSelection(types::TraceTime(0), data->getTotalRuntime());
}

void MainWindow::openFilterPopup() {
    FilterPopup filterPopup(data->getSettings()->getFilter());

    auto connection = connect(&filterPopup, SIGNAL(filterChanged(Filter)), this->data, SLOT(setFilter(Filter)));

    filterPopup.exec();

    disconnect(connection);
}

void MainWindow::openNewTrace() {
    this->promptFile();
    this->loadTrace();
}

