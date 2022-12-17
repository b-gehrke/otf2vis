#include "MainWindow.hpp"
#include "src/readercallbacks.hpp"
#include "src/ui/views/TraceInformationDock.hpp"
#include "src/ui/views/License.hpp"

#include "QDropEvent"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QStringListModel>
#include <QWidget>
#include <QLineEdit>
#include <QErrorMessage>
#include <QIntValidator>

MainWindow::MainWindow(QString path, QWidget *parent) : filePath(std::move(path)) {
    // TODO delete object on close properly
    //setAttribute(Qt::WA_DeleteOnClose);

    if (filePath.isEmpty()) {
        getTraceFilePath();
    }
    loadTraceFile(filePath);

    createMenus();
    // contains buttons and overview
    createToolBars();
    // contains details
    createDockWidgets();
    // contains list of processes
    createCentralWidget();
}

void MainWindow::createMenus() {
    /// File menu
    // "Open Trace" menu entry
    auto openTraceAction = new QAction(tr("&Open..."), this);
    openTraceAction->setShortcut(tr("Ctrl+O"));
    connect(openTraceAction, SIGNAL(triggered()), this, SLOT(openTrace()));

    // "Quit" menu entry
    auto quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openTraceAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    /// View Menu
    // "Filter" menu entry
    auto filterAction = new QAction(tr("&Filter"));
    // TODO S for sieve? what might be more intuitive?
    filterAction->setShortcut(tr("Ctrl+S"));
    // TODO add actual slot
    connect(filterAction, SIGNAL(triggered()), this, SLOT(openFilterPopup()));

    // "Search" menu entry
    auto searchAction = new QAction(tr("&Find"));
    searchAction->setShortcut(tr("Ctrl+F"));
    // TODO add actual slot
    connect(searchAction, SIGNAL(triggered()), this, SLOT(openFilterPopup()));

    auto viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(filterAction);
    viewMenu->addAction(searchAction);

    /// Window menu
    // "Minimize" menu entry
    auto minimizeAction = new QAction(tr("&Minimize"), this);
    minimizeAction->setShortcut(tr("Ctrl+M"));
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

    auto windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(minimizeAction);

    /// Help menu
    // "View license" menu entry
    auto aboutAction = new QAction(tr("&View license"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(openLicenseView()));

    auto helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBars() {
    // Top toolbar contains preview/control of whole trace
    topToolbar = new QToolBar(this);
    topToolbar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, topToolbar);

    preview = new view::Preview(trace, this);
    topToolbar->addWidget(preview);

    // Bottom toolbar contains control fields
    bottomToolbar = new QToolBar(this);
    bottomToolbar->setMovable(false);
    addToolBar(Qt::BottomToolBarArea, bottomToolbar);

    auto containerWidget = new QWidget(bottomToolbar);
    auto containerLayout = new QHBoxLayout(containerWidget);
    containerWidget->setLayout(containerLayout);

    // TODO customize: (e.g. start < end; not out of bounds)
    auto validator = new QIntValidator(containerWidget);

    containerLayout->addWidget(new QLabel("Start:", containerWidget));
    intervalBegin = new QLineEdit(QString::number(viewStart), containerWidget);
    intervalBegin->setValidator(validator);
    connect(intervalBegin, SIGNAL(returnPressed()), this, SLOT(applyIntervalText()));
//    connect(this, SIGNAL(selectionUpdated()), intervalBegin,
//            SLOT([this]{intervalBegin->setText(QString::number(this->viewStart));}));
    containerLayout->addWidget(intervalBegin);

    containerLayout->addWidget(new QLabel("End:", containerWidget));
    intervalEnd = new QLineEdit(QString::number(viewEnd), containerWidget);
    connect(intervalEnd, SIGNAL(returnPressed()), this, SLOT(applyIntervalText()));
//    connect(this, SIGNAL(selectionUpdated()), intervalEnd,
//            SLOT([this]{intervalEnd->setText(QString::number(this->viewEnd));});
    intervalEnd->setValidator(validator);
    containerLayout->addWidget(intervalEnd);

    bottomToolbar->addWidget(containerWidget);
}

void MainWindow::createDockWidgets() {
    auto traceInformation = new view::TraceInformationDock(trace, this);
    addDockWidget(Qt::RightDockWidgetArea, traceInformation);
}

void MainWindow::createCentralWidget() {
    traceList = new view::TraceList(selection, this);
    setCentralWidget(traceList);

    connect(this, SIGNAL(selectionUpdated()), traceList, SLOT(updateView()));
}

void MainWindow::updateView() {
//    viewStart = start;
//    viewEnd = end;
    traceList->updateView();
    traceList->update();
    update();
}

void MainWindow::loadTraceFile(const QString &path) {
    reader = std::make_shared<otf2::reader::reader>(filePath.toStdString());
    reader_callbacks = std::make_shared<ReaderCallbacks>(*reader);

    reader->set_callback(*reader_callbacks);
    reader->read_definitions();
    reader->read_events();

    trace = std::make_shared<FileTrace>(*reader_callbacks->getSlots(), *reader_callbacks->getCommunications(),
                                        *reader_callbacks->getCollectiveCommunications(), reader_callbacks->duration());

    viewStart = trace->getRuntime().count()/4;
    // TODO create getter and setter
    viewEnd = trace->getRuntime().count()/2;

    selection = std::make_shared<SubTrace>(trace->getSlots(), trace->getCommunications(),
                                           trace->getCollectiveCommunications(),
                                           trace->getRuntime(), trace->getStartTime());
//    auto newTrace = trace->subtrace(otf2::chrono::duration(viewStart), otf2::chrono::duration(viewEnd));
//    selection = std::make_shared<SubTrace>(newTrace->getSlots(), newTrace->getCommunications(),
//                                           newTrace->getCollectiveCommunications(),
//                                           newTrace->getRuntime(), newTrace->getStartTime());

}

QString MainWindow::getTraceFilePath() {
    auto newFilePath = QFileDialog::getOpenFileName(this, QFileDialog::tr("Open trace"), QString(),
                                                    QFileDialog::tr("OTF Traces (*.otf *.otf2)"));

    // TODO this is not really a great way to deal with that
    if (newFilePath.isEmpty()) {
        auto errorMsg = new QErrorMessage(this);
        errorMsg->showMessage("The chosen file is invalid!");
    } else {
        filePath = newFilePath;
    }

    return filePath;
}

void MainWindow::openLicenseView() {
    licenseWindow = new view::License(nullptr);
    licenseWindow->show();
}

void MainWindow::openTrace() {
    filePath = getTraceFilePath();
    loadTraceFile(filePath);
}

void MainWindow::applyIntervalText() {
    viewStart = intervalBegin->text().toLongLong();
    viewEnd = intervalEnd->text().toLongLong();

    // TODO copy constructor please
    //selection = trace->subtrace(otf2::chrono::duration(viewStart), otf2::chrono::duration(viewEnd));
    auto newTrace = trace->subtrace(otf2::chrono::duration(viewStart), otf2::chrono::duration(viewEnd));
    selection = std::make_shared<SubTrace>(newTrace->getSlots(), newTrace->getCommunications(),
                               newTrace->getCollectiveCommunications(),
                               newTrace->getRuntime(), newTrace->getStartTime());

    Q_EMIT selectionUpdated();
}