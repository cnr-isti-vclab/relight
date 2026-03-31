#include "mainwindow.h"

#include "../src/imageset.h"
#include "../src/brdf/init_normals.h"
#include "../src/brdf/brdf_optimizer.h"
#include "../relightlab/canvas.h"
#include "diagnosticpanel.h"
#include "normalspherewidget.h"
#include "reflectanceview.h"
#include "rusinview.h"

#include <QtConcurrent>
#include <QFutureWatcher>

#include <QApplication>
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPixmap>
#include <QPainter>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include <vector>
#include <Eigen/Core>

// ---------------------------------------------------------------------------
// Placeholder panel widgets — replaced in later phases with real implementations
// ---------------------------------------------------------------------------

class DatasetPanel : public QWidget {
    Q_OBJECT
public:
    explicit DatasetPanel(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        
        canvas = new Canvas(this);
        scene  = new QGraphicsScene(this);
        canvas->setScene(scene);
        canvas->setMinimumWidth(320);
        canvas->setCursor(Qt::CrossCursor);
        layout->addWidget(canvas, 1);
        
        infoLabel = new QLabel("No dataset loaded", this);
        infoLabel->setAlignment(Qt::AlignCenter);
        infoLabel->setStyleSheet("color: #888; font-size: 11px; padding: 2px;");
        layout->addWidget(infoLabel, 0);
        
        connect(canvas, &Canvas::clicked, this, [this](QPoint vp_pt) {
            if (!pixmapItem)
                return;
            QPointF sp = canvas->mapToScene(vp_pt);
            int x = qBound(0, qRound(sp.x()), imageset.image_width  - 1);
            int y = qBound(0, qRound(sp.y()), imageset.image_height - 1);
            emit pixelClicked(x, y);
        });
    }
    
    void loadFolder(const QString &path) {
        QDir dir(path);

        // Find and parse the .lp file for light directions
        QStringList lps = dir.entryList(QStringList() << "*.lp");
        if (lps.isEmpty()) {
            infoLabel->setText("No .lp file found in: " + path);
            return;
        }
        dome = Dome();
        dome.parseLP(dir.filePath(lps[0]));

        // Find image files and open decoders
        if (!imageset.initFromFolder(path.toUtf8().constData())) {
            infoLabel->setText("Failed to load images from: " + path);
            return;
        }

        // Apply light directions from dome
        imageset.initFromDome(dome);
        imageset.setColorProfileMode(COLOR_PROFILE_LINEAR_RGB);

        current_idx = 0;
        showImage(0);
    }
    
    void showImage(int idx) {
        if (imageset.images.isEmpty())
            return;
        current_idx = qBound(0, idx, int(imageset.images.size()) - 1);
        QImage img = imageset.readImageCropped(size_t(current_idx));
        if (img.isNull()) {
            infoLabel->setText("Failed to read image");
            return;
        }
        QPixmap px = QPixmap::fromImage(img);
        if (!pixmapItem) {
            pixmapItem = scene->addPixmap(px);
        } else {
            pixmapItem->setPixmap(px);
        }
        scene->setSceneRect(pixmapItem->boundingRect());
        
        int total = int(imageset.images.size());
        QString info = QString("Image %1/%2").arg(current_idx + 1).arg(total);
        const auto &lights = imageset.lights();
        if (current_idx < int(lights.size())) {
            auto l = lights[size_t(current_idx)];
            info += QString("   Light: (%1, %2, %3)")
                        .arg(double(l.x()), 0, 'f', 2)
                        .arg(double(l.y()), 0, 'f', 2)
                        .arg(double(l.z()), 0, 'f', 2);
        }
        infoLabel->setText(info);
        emit lightIndexChanged(current_idx);
        canvas->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
    
    void prev() {
        if (imageset.images.isEmpty()) return;
        int n = int(imageset.images.size());
        showImage((current_idx - 1 + n) % n);
    }
    
    void next() {
        if (imageset.images.isEmpty()) return;
        int n = int(imageset.images.size());
        showImage((current_idx + 1) % n);
    }
    
    void fit() {
        if (!scene->sceneRect().isEmpty())
            canvas->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
    
signals:
    void pixelClicked(int x, int y);
    void lightIndexChanged(int idx);
    
public:
    ImageSet imageset;
    Dome dome;
    int current_idx = 0;
    Canvas *canvas             = nullptr;
    QGraphicsScene *scene      = nullptr;
    QGraphicsPixmapItem *pixmapItem = nullptr;
    QLabel *infoLabel          = nullptr;
};

class ResultsPanel : public QTabWidget {
    Q_OBJECT
public:
    explicit ResultsPanel(QWidget *parent = nullptr) : QTabWidget(parent) {
        addTab(makePlaceholder("Maps\n(albedo · normal · roughness · metallic · init maps)"), "Maps");
        diagnosticPanel = new DiagnosticPanel(this);
        addTab(diagnosticPanel, "Diagnostics");
        addTab(makePlaceholder("Per-light render\n(original vs. BRDF re-render)"), "Per-light");
        reflectanceView = new ReflectanceView(this);
        addTab(reflectanceView, "Reflectance");
        rusinView = new RusinkiewiczView(this);
        addTab(rusinView, "Rusinkiewicz");
    }
    DiagnosticPanel    *diagnosticPanel    = nullptr;
    ReflectanceView    *reflectanceView    = nullptr;
    RusinkiewiczView   *rusinView          = nullptr;
private:
    static QWidget *makePlaceholder(const QString &text) {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        auto *lbl = new QLabel(text);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("color: #888; font-size: 13px;");
        l->addWidget(lbl);
        return w;
    }
};

class ParamsPanel : public QWidget {
public:
    explicit ParamsPanel(QWidget *parent = nullptr) : QWidget(parent) {
        auto *l = new QVBoxLayout(this);
        auto *lbl = new QLabel(
            "<b>Parameters</b><br>"
            "Patch size, thresholds, optimizer flags — coming in next phase.", this);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("color: #888; font-size: 12px;");
        l->addWidget(lbl);
        l->addStretch();
        setFixedWidth(300);
    }
};

// Need to pull in the moc output for the inline Q_OBJECT classes above
#include "mainwindow.moc"

// ---------------------------------------------------------------------------
// PixelInfoCard
// ---------------------------------------------------------------------------

PixelInfoCard::PixelInfoCard(QWidget *parent) : QWidget(parent) {
    setVisible(false);
    
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);
    
    // ---- Header ----
    coords_label = new QLabel("Pixel (—, —)", this);
    coords_label->setStyleSheet("font-weight: bold; font-size: 13px;");
    root->addWidget(coords_label);
    
    // ---- Init normal section ----
    auto *initBox = new QGroupBox("Initial estimate (init_normal)", this);
    auto *ig = new QGridLayout(initBox);
    ig->setColumnStretch(1, 1);
    ig->setSpacing(3);
    
    init_normal_swatch = new QLabel(initBox);
    init_normal_swatch->setFixedSize(40, 40);
    ig->addWidget(init_normal_swatch, 0, 0, 3, 1);
    
    init_metallic_label  = new QLabel("metallic: —", initBox);
    init_hf_label        = new QLabel("highlight fraction: —", initBox);
    init_pr_label        = new QLabel("peak ratio: —", initBox);
    init_var_label       = new QLabel("Lambertian variance: —", initBox);
    ig->addWidget(init_metallic_label,  0, 1);
    ig->addWidget(init_hf_label,        1, 1);
    ig->addWidget(init_pr_label,        2, 1);
    ig->addWidget(init_var_label,       3, 1, 1, 2);
    root->addWidget(initBox);
    
    // ---- Fit section ----
    auto *fitBox = new QGroupBox("Ceres fit result", this);
    auto *fg = new QGridLayout(fitBox);
    fg->setColumnStretch(1, 1);
    fg->setSpacing(3);
    
    fit_normal_swatch   = new QLabel(fitBox);
    fit_normal_swatch->setFixedSize(40, 40);
    fit_albedo_swatch   = new QLabel(fitBox);
    fit_albedo_swatch->setFixedSize(40, 40);
    fg->addWidget(fit_normal_swatch,  0, 0, 2, 1);
    fg->addWidget(fit_albedo_swatch,  0, 2, 2, 1);
    
    fit_roughness_label = new QLabel("roughness: —", fitBox);
    fit_metallic_label  = new QLabel("metallic: —", fitBox);
    fg->addWidget(fit_roughness_label, 0, 1);
    fg->addWidget(fit_metallic_label,  1, 1);
    root->addWidget(fitBox);
    
    clearInitResults();
    clearFitResults();
}

void PixelInfoCard::setCoords(int x, int y) {
    coords_label->setText(QString("Pixel (%1, %2)").arg(x).arg(y));
    setVisible(true);
}

void PixelInfoCard::clearInitResults() {
    init_metallic_label->setText("metallic: —");
    init_hf_label->setText("highlight fraction: —");
    init_pr_label->setText("peak ratio: —");
    init_var_label->setText("Lambertian variance: —");
    init_normal_swatch->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f));
}

void PixelInfoCard::clearFitResults() {
    fit_roughness_label->setText("roughness: —");
    fit_metallic_label->setText("metallic: —");
    fit_normal_swatch->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f));
    fit_albedo_swatch->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f));
}

void PixelInfoCard::setInitResults(bool is_metallic, float highlight_fraction,
                                   float peak_ratio, float lambertian_variance,
                                   float nx, float ny, float nz) {
    init_metallic_label->setText(
        QString("metallic: <b>%1</b>").arg(is_metallic ? "yes" : "no"));
    init_metallic_label->setStyleSheet(
        is_metallic ? "color: #e8a000;" : "");
    init_hf_label->setText(
        QString("highlight fraction: %1").arg(double(highlight_fraction), 0, 'f', 3));
    init_pr_label->setText(
        QString("peak ratio: %1").arg(double(peak_ratio), 0, 'f', 2));
    init_var_label->setText(
        QString("Lambertian variance: %1").arg(double(lambertian_variance), 0, 'e', 2));
    init_normal_swatch->setPixmap(normalSwatch(nx, ny, nz));
}

void PixelInfoCard::setFitResults(float nx, float ny, float nz,
                                  float roughness, float metallic,
                                  float albedo_r, float albedo_g, float albedo_b) {
    fit_roughness_label->setText(
        QString("roughness: %1").arg(double(roughness), 0, 'f', 3));
    fit_metallic_label->setText(
        QString("metallic: %1").arg(double(metallic), 0, 'f', 3));
    fit_normal_swatch->setPixmap(normalSwatch(nx, ny, nz));
    fit_albedo_swatch->setPixmap(colorSwatch(albedo_r, albedo_g, albedo_b));
}

QPixmap PixelInfoCard::normalSwatch(float nx, float ny, float nz, int size) {
    // Normal → RGB encoding: r=(nx+1)/2, g=(ny+1)/2, b=(nz+1)/2
    float r = std::clamp((nx + 1.0f) * 0.5f, 0.0f, 1.0f);
    float g = std::clamp((ny + 1.0f) * 0.5f, 0.0f, 1.0f);
    float b = std::clamp((nz + 1.0f) * 0.5f, 0.0f, 1.0f);
    return colorSwatch(r, g, b, size);
}

QPixmap PixelInfoCard::colorSwatch(float r, float g, float b, int size) {
    QPixmap px(size, size);
    px.fill(QColor(
        std::clamp(int(r * 255), 0, 255),
        std::clamp(int(g * 255), 0, 255),
        std::clamp(int(b * 255), 0, 255)));
    // Draw a thin border
    QPainter p(&px);
    p.setPen(QColor(0, 0, 0, 80));
    p.drawRect(0, 0, size - 1, size - 1);
    return px;
}

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settings("VCG", "BrdfLab")
{
    setWindowTitle("BrdfLab — BRDF Explorer");
    
    createActions();
    createMenuBar();
    createToolBar();
    createPanels();
    createStatusBar();
    restoreLayout();
}

MainWindow::~MainWindow() = default;

void MainWindow::createActions() {
    actOpen = new QAction(QIcon::fromTheme("document-open"), "Open folder…", this);
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::openFolder);
    
    actPrev = new QAction(QIcon::fromTheme("go-previous"), "Previous image", this);
    actPrev->setShortcut(Qt::Key_Left);
    
    actNext = new QAction(QIcon::fromTheme("go-next"), "Next image", this);
    actNext->setShortcut(Qt::Key_Right);
    
    actFit = new QAction(QIcon::fromTheme("zoom-fit-best"), "Fit view", this);
    actFit->setShortcut(Qt::Key_F);
    
    actRecompute = new QAction(QIcon::fromTheme("view-refresh"), "Recompute", this);
    actRecompute->setShortcut(Qt::Key_R);
    actRecompute->setEnabled(false);
}

void MainWindow::createToolBar() {
    QToolBar *tb = addToolBar("Main");
    tb->setObjectName("mainToolBar");
    tb->setMovable(false);
    tb->addAction(actOpen);
    tb->addSeparator();
    tb->addAction(actPrev);
    tb->addAction(actNext);
    tb->addSeparator();
    tb->addAction(actFit);
    tb->addSeparator();
    tb->addAction(actRecompute);
}

void MainWindow::createMenuBar() {
    QMenu *file = menuBar()->addMenu("File");
    file->addAction(actOpen);
    file->addSeparator();
    file->addAction("Quit", qApp, &QApplication::quit, QKeySequence::Quit);
    
    QMenu *view = menuBar()->addMenu("View");
    view->addAction(actFit);
    view->addAction(actPrev);
    view->addAction(actNext);
    
    QMenu *run = menuBar()->addMenu("Run");
    run->addAction(actRecompute);
}

void MainWindow::createPanels() {
    // ---- Left: dataset ----
    datasetPanel = new DatasetPanel(this);
    connect(datasetPanel, &DatasetPanel::pixelClicked,
            this, &MainWindow::onPixelClicked);
    connect(actPrev, &QAction::triggered,
            datasetPanel, [this]{ datasetPanel->prev(); });
    connect(actNext, &QAction::triggered,
            datasetPanel, [this]{ datasetPanel->next(); });
    connect(actFit, &QAction::triggered,
            datasetPanel, [this]{ datasetPanel->fit(); });
    
    // ---- Centre: results tabs ----
    resultsPanel = new ResultsPanel(this);
    
    // ---- Right: params + pixel info card ----
    rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    pixelInfoCard = new PixelInfoCard(rightPanel);
    rightLayout->addWidget(pixelInfoCard);

    normalSphereWidget = new NormalSphereWidget(rightPanel);
    rightLayout->addWidget(normalSphereWidget);
    connect(normalSphereWidget, &NormalSphereWidget::normalChanged,
            this, [this](const Eigen::Vector3f &n) {
                if (resultsPanel && resultsPanel->diagnosticPanel)
                    resultsPanel->diagnosticPanel->setNormal(n);
                if (resultsPanel && resultsPanel->rusinView)
                    resultsPanel->rusinView->setNormal(n);
            });
    connect(resultsPanel->rusinView, &RusinkiewiczView::lightSelected,
            this, [this](int idx) { datasetPanel->showImage(idx); });
    connect(resultsPanel->diagnosticPanel, &DiagnosticPanel::lightSelected,
            this, [this](int idx) { datasetPanel->showImage(idx); });

    // Scroll area wrapping the params panel so it doesn't crush on small screens
    paramsPanel = new ParamsPanel(rightPanel);
    auto *scroll = new QScrollArea(rightPanel);
    scroll->setWidget(paramsPanel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    rightLayout->addWidget(scroll);
    
    // ---- Splitter ----
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("mainSplitter");
    splitter->addWidget(datasetPanel);
    splitter->addWidget(resultsPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);  // dataset: fixed
    splitter->setStretchFactor(1, 1);  // results: stretchy
    splitter->setStretchFactor(2, 0);  // params: fixed
    
    setCentralWidget(splitter);
    resize(1440, 900);
}

void MainWindow::createStatusBar() {
    statusFolder = new QLabel("No folder open", this);
    statusPixel  = new QLabel("", this);
    statusState  = new QLabel("Ready", this);
    
    statusBar()->addWidget(statusFolder, 3);
    statusBar()->addPermanentWidget(statusPixel, 1);
    statusBar()->addPermanentWidget(statusState, 1);
}

void MainWindow::openFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Open image folder",
        settings.value("lastFolder", QDir::homePath()).toString());
    if (dir.isEmpty())
        return;
    
    // Stop any background computation that references the current imageset.
    cancelCompute.store(true);
    if (computeWatcher && computeWatcher->isRunning())
        computeWatcher->waitForFinished();
    cancelCompute.store(false);
    
    settings.setValue("lastFolder", dir);
    statusFolder->setText(dir);
    actRecompute->setEnabled(true);
    datasetPanel->loadFolder(dir);
}

void MainWindow::onPixelClicked(int x, int y) {
    statusPixel->setText(QString("(%1, %2)").arg(x).arg(y));
    pixelInfoCard->setCoords(x, y);
    pixelInfoCard->clearInitResults();
    pixelInfoCard->clearFitResults();
    
    ImageSet &imageset = datasetPanel->imageset;
    int n = int(imageset.size());
    std::vector<Eigen::Vector3f> lights = imageset.lights();
    if (n == 0 || lights.empty())
        return;
    
    // Calculate luminosities and colors for each light
    std::vector<float> luminosities(n);
	std::vector<Eigen::Vector3f> colors(n);
    for (int j = 0; j < n; j++) {
        QImage img = imageset.readImageCropped(size_t(j));
        QRgb rgb = img.pixel(x, y);
        Eigen::Vector3f col(qRed(rgb) / 255.0f, qGreen(rgb) / 255.0f, qBlue(rgb) / 255.0f);
        colors[size_t(j)] = col;
        luminosities[j] = 0.299f * col.x() + 0.587f * col.y() + 0.114f * col.z();
    }
    if (resultsPanel && resultsPanel->diagnosticPanel)
        resultsPanel->diagnosticPanel->update(luminosities, lights);
    if (resultsPanel && resultsPanel->rusinView)
        resultsPanel->rusinView->setData(colors, lights);
    if (resultsPanel && resultsPanel->reflectanceView)
        resultsPanel->reflectanceView->setData(colors, lights);
    if (normalSphereWidget)
        normalSphereWidget->clear();
    
    // Cancel and wait for any running computation before starting a new one.
    cancelCompute.store(true);
    if (computeWatcher && computeWatcher->isRunning())
        computeWatcher->waitForFinished();
    cancelCompute.store(false);
    
    if (!computeWatcher) {
        computeWatcher = new QFutureWatcher<void>(this);
        connect(computeWatcher, &QFutureWatcher<void>::finished,
                this, [this]{ statusState->setText("Ready"); });
    }
    
    clickedX = x;
    clickedY = y;
    statusState->setText("Computing…");
    
    PixelInfoCard      *card      = pixelInfoCard;
    NormalSphereWidget *sphere    = normalSphereWidget;
    DiagnosticPanel    *diagPanel = resultsPanel ? resultsPanel->diagnosticPanel : nullptr;
    RusinkiewiczView   *rusinView = resultsPanel ? resultsPanel->rusinView       : nullptr;
    QLabel *stateLabel   = statusState;
    std::atomic<bool> *cancel = &cancelCompute;
    
    QFuture<void> future = QtConcurrent::run([=, &imageset]() {
        // --- Build Pixel: read each image and sample the clicked position ---
        Pixel p;
        p.x = x;
        p.y = y;
        p.resize(size_t(n));
        for (int j = 0; j < n; j++) {
            if (cancel->load()) return;
            QImage img = imageset.readImageCropped(size_t(j));
            QRgb rgb = img.pixel(x, y);
            p[size_t(j)] = Color3f(qRed(rgb)   / 255.0f,
                                   qGreen(rgb) / 255.0f,
                                   qBlue(rgb)  / 255.0f);
        }
        if (cancel->load()) return;
        
        // --- Fast heuristic: init_normal ---
        brdf::InitNormalResult init = brdf::init_normal(p, lights);

        QMetaObject::invokeMethod(card, [card, sphere, diagPanel, rusinView, init]{
            card->setInitResults(
                init.is_metallic, init.highlight_fraction, init.peak_ratio,
                init.lambertian_variance,
                init.normal.x(), init.normal.y(), init.normal.z());
            if (sphere)
                sphere->setNormal(init.normal);
            if (diagPanel)
                diagPanel->setNormal(init.normal);
            if (rusinView)
                rusinView->setNormal(init.normal);
        }, Qt::QueuedConnection);
        
        if (cancel->load()) return;
        
        // --- Albedo estimate (median across lights) ---
        Eigen::Vector3f albedo = {0.1f, 0.1f, 0.1f};
        for (int k = 0; k < 3; k++) {
            std::vector<float> channel(static_cast<size_t>(n));
            for (int j = 0; j < n; j++)
                channel[j] = p[j][k];
            std::nth_element(channel.begin(), channel.begin() + n / 2, channel.end());
            albedo[k] = std::max(channel[size_t(n / 2)], 0.001f);
        }
        
        if (cancel->load()) return;
        
        // --- Slow Ceres fit: optimize_brdf_pixel ---
        float init_metallic = init.is_metallic ? 0.5f : 0.0f;
        brdf::BrdfFitResult fit = brdf::optimize_brdf_pixel(
            p, lights,
            init.normal, albedo,
            0.3f, init_metallic,
            4.0f, /*optimize_normal=*/true, /*optimize_albedo=*/true);
        
        if (cancel->load()) return;
        QMetaObject::invokeMethod(card, [card, fit]{
            card->setFitResults(
                fit.normal.x(), fit.normal.y(), fit.normal.z(),
                fit.roughness, fit.metallic,
                fit.albedo.x(), fit.albedo.y(), fit.albedo.z());
        }, Qt::QueuedConnection);
    });
    
    computeWatcher->setFuture(future);
}

void MainWindow::saveLayout() {
    settings.setValue("geometry",     saveGeometry());
    settings.setValue("windowState",  saveState());
    settings.setValue("splitterSizes", splitter->saveState());
}

void MainWindow::restoreLayout() {
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());
    if (settings.contains("windowState"))
        restoreState(settings.value("windowState").toByteArray());
    if (settings.contains("splitterSizes"))
        splitter->restoreState(settings.value("splitterSizes").toByteArray());
    else
        splitter->setSizes({420, 720, 300}); // defaults
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveLayout();
    event->accept();
}
