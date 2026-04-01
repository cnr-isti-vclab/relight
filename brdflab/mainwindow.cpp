#include "mainwindow.h"

#include "../src/imageset.h"
#include "../src/brdf/init_normals.h"
#include "../src/brdf/init_normals_patch.h"
#include "../src/brdf/brdf_optimizer.h"
#include "../relightlab/canvas.h"
#include "rusinview.h"
#include "reflectanceview.h"
#include "diagnosticpanel.h"
#include "normalspherewidget.h"

#include <QtConcurrent>
#include <QFutureWatcher>

#include <QApplication>
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
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
// Window size for BRDF fitting (must be odd; controls the NxN pixel patch
// fitted around the clicked pixel — boundary clamping ensures the full patch
// always fits inside the image).
static constexpr int kBrdfWindowSize = 5;
static constexpr int kBrdfWindowHalf = kBrdfWindowSize / 2;

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
        canvas->max_scale = 64.0;
        layout->addWidget(canvas, 1);

        markerItem = new QGraphicsRectItem();
        QPen markerPen(Qt::red);
        markerPen.setCosmetic(true);
        markerItem->setPen(markerPen);
        markerItem->setBrush(Qt::NoBrush);
        markerItem->setVisible(false);
        markerItem->setZValue(1);
        scene->addItem(markerItem);

        infoLabel = new QLabel("No dataset loaded", this);
        infoLabel->setAlignment(Qt::AlignCenter);
        infoLabel->setStyleSheet("color: #888; font-size: 11px; padding: 2px;");
        layout->addWidget(infoLabel, 0);
        
        connect(canvas, &Canvas::clicked, this, [this](QPoint vp_pt) {
            if (!pixmapItem)
                return;
            QPointF sp = canvas->mapToScene(vp_pt);
            // Clamp so the full kBrdfWindowSize patch always lies inside the image
            int x = qBound(kBrdfWindowHalf, qRound(sp.x()),
                           imageset.image_width  - 1 - kBrdfWindowHalf);
            int y = qBound(kBrdfWindowHalf, qRound(sp.y()),
                           imageset.image_height - 1 - kBrdfWindowHalf);
            markerItem->setRect(QRectF(x - kBrdfWindowHalf,
                                       y - kBrdfWindowHalf,
                                       kBrdfWindowSize, kBrdfWindowSize));
            markerItem->setVisible(true);
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
		showImage(0, true);
    }
    
	void showImage(int idx, bool fitView = false) {
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
        if (fitView)
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
    QGraphicsRectItem   *markerItem  = nullptr;
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
    auto *fitBox = new QGroupBox("Fit results", this);
    auto *fg = new QVBoxLayout(fitBox);
    fg->setSpacing(4);

    // Build a labeled 5x5 grid of color swatches for one map.
    const int cellSz = 26;
    auto makeGrid = [&](std::vector<QLabel*> &cells, const QString &label) -> QWidget* {
        auto *w  = new QWidget(fitBox);
        auto *vl = new QVBoxLayout(w);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(1);
        auto *lbl = new QLabel(label, w);
        lbl->setStyleSheet("font-size: 10px; color: #aaa; font-weight: bold;");
        vl->addWidget(lbl);
        auto *gridW = new QWidget(w);
        auto *gl = new QGridLayout(gridW);
        gl->setSpacing(1);
        gl->setContentsMargins(0, 0, 0, 0);
        const int count = kBrdfWindowSize * kBrdfWindowSize;
        cells.resize(size_t(count));
        for (int i = 0; i < count; ++i) {
            cells[size_t(i)] = new QLabel(gridW);
            cells[size_t(i)]->setFixedSize(cellSz, cellSz);
            cells[size_t(i)]->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, cellSz));
            gl->addWidget(cells[size_t(i)], i / kBrdfWindowSize, i % kBrdfWindowSize);
        }
        vl->addWidget(gridW);
        return w;
    };

    // 2×2 arrangement: Normal | Albedo / Roughness | Metallic
    auto *mapsW = new QWidget(fitBox);
    auto *mapsLayout = new QGridLayout(mapsW);
    mapsLayout->setSpacing(8);
    mapsLayout->setContentsMargins(0, 0, 0, 0);
    mapsLayout->addWidget(makeGrid(fit_normal_cells,    "Normal"),    0, 0);
    mapsLayout->addWidget(makeGrid(fit_albedo_cells,    "Albedo"),    0, 1);
    mapsLayout->addWidget(makeGrid(fit_roughness_cells, "Roughness"), 1, 0);
    mapsLayout->addWidget(makeGrid(fit_metallic_cells,  "Metallic"),  1, 1);
    fg->addWidget(mapsW);

    // Shared material (patch) summary — updated by setPatchMaterialResult
    auto *patchHdr = new QLabel("Shared material (patch):", fitBox);
    patchHdr->setStyleSheet("font-size: 10px; color: #aaa; font-weight: bold; margin-top: 4px;");
    fg->addWidget(patchHdr);

    auto *sharedRow = new QWidget(fitBox);
    auto *sharedH   = new QHBoxLayout(sharedRow);
    sharedH->setContentsMargins(0, 0, 0, 0);
    sharedH->setSpacing(6);

    patch_albedo_swatch = new QLabel(sharedRow);
    patch_albedo_swatch->setFixedSize(40, 40);
    patch_albedo_swatch->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, 40));
    sharedH->addWidget(patch_albedo_swatch);

    auto *sharedTxtW = new QWidget(sharedRow);
    auto *sharedTxtV = new QVBoxLayout(sharedTxtW);
    sharedTxtV->setContentsMargins(0, 0, 0, 0);
    sharedTxtV->setSpacing(1);
    fit_roughness_label = new QLabel("roughness: —", sharedTxtW);
    fit_metallic_label  = new QLabel("metallic: —",  sharedTxtW);
    sharedTxtV->addWidget(fit_roughness_label);
    sharedTxtV->addWidget(fit_metallic_label);
    sharedH->addWidget(sharedTxtW);
    sharedH->addStretch();
    fg->addWidget(sharedRow);
    root->addWidget(fitBox);

    // ---- Raw patch section ----
    auto *rawBox = new QGroupBox("Raw observed patch", this);
    auto *rl = new QVBoxLayout(rawBox);
    rl->setSpacing(2);
    raw_patch_light_label = new QLabel("Light: —", rawBox);
    raw_patch_light_label->setStyleSheet("font-size: 10px; color: #aaa;");
    rl->addWidget(raw_patch_light_label);
    auto *rawGridW = new QWidget(rawBox);
    auto *rgl = new QGridLayout(rawGridW);
    rgl->setSpacing(1);
    rgl->setContentsMargins(0, 0, 0, 0);
    const int rawCount = kBrdfWindowSize * kBrdfWindowSize;
    raw_patch_cells.resize(rawCount);
    for (int i = 0; i < rawCount; ++i) {
        raw_patch_cells[i] = new QLabel(rawGridW);
        raw_patch_cells[i]->setFixedSize(26, 26);
        raw_patch_cells[i]->setPixmap(colorSwatch(0.1f, 0.1f, 0.1f, 26));
        rgl->addWidget(raw_patch_cells[i], i / kBrdfWindowSize, i % kBrdfWindowSize);
    }
    rl->addWidget(rawGridW);
    root->addWidget(rawBox);

    clearInitResults();
    clearFitResults();
}

void PixelInfoCard::setCoords(int x, int y) {
    coords_label->setText(QString("Pixel (%1, %2)").arg(x).arg(y));
    m_window_colors.clear();
    raw_patch_light_label->setText("Light: —");
    for (auto *c : raw_patch_cells) c->setPixmap(colorSwatch(0.1f, 0.1f, 0.1f, 26));
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
    patch_albedo_swatch->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, 40));
    const int sz = 26;
    for (auto *c : fit_normal_cells)    c->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, sz));
    for (auto *c : fit_albedo_cells)    c->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, sz));
    for (auto *c : fit_roughness_cells) c->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, sz));
    for (auto *c : fit_metallic_cells)  c->setPixmap(colorSwatch(0.3f, 0.3f, 0.3f, sz));
}

void PixelInfoCard::setWindowData(
        const std::vector<std::vector<Eigen::Vector3f>>& per_light_colors) {
    m_window_colors = per_light_colors;
    if (!m_window_colors.empty())
        showWindowLight(0);
}

void PixelInfoCard::showWindowLight(int light_idx) {
    if (m_window_colors.empty() ||
        light_idx < 0 ||
        light_idx >= int(m_window_colors.size()))
        return;
    raw_patch_light_label->setText(QString("Light: %1").arg(light_idx));
    const auto& colors = m_window_colors[light_idx];
    for (int wi = 0; wi < int(raw_patch_cells.size()) && wi < int(colors.size()); ++wi) {
        const Eigen::Vector3f& c = colors[wi];
        raw_patch_cells[wi]->setPixmap(colorSwatch(c.x(), c.y(), c.z(), 26));
    }
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

void PixelInfoCard::setFitResults(int wi,
                                  float nx, float ny, float nz,
                                  float roughness, float metallic,
                                  float albedo_r, float albedo_g, float albedo_b) {
    const int sz = 26;
    if (wi >= 0 && wi < int(fit_normal_cells.size()))
        fit_normal_cells[size_t(wi)]->setPixmap(normalSwatch(nx, ny, nz, sz));
    if (wi >= 0 && wi < int(fit_albedo_cells.size()))
        fit_albedo_cells[size_t(wi)]->setPixmap(colorSwatch(albedo_r, albedo_g, albedo_b, sz));
    if (wi >= 0 && wi < int(fit_roughness_cells.size()))
        fit_roughness_cells[size_t(wi)]->setPixmap(colorSwatch(roughness, roughness, roughness, sz));
    if (wi >= 0 && wi < int(fit_metallic_cells.size()))
        fit_metallic_cells[size_t(wi)]->setPixmap(colorSwatch(metallic, metallic, metallic, sz));
}

void PixelInfoCard::setPatchMaterialResult(
        float roughness, float metallic,
        float albedo_r, float albedo_g, float albedo_b) {
    fit_roughness_label->setText(
        QString("roughness: %1").arg(double(roughness), 0, 'f', 3));
    fit_metallic_label->setText(
        QString("metallic: %1").arg(double(metallic), 0, 'f', 3));
    patch_albedo_swatch->setPixmap(colorSwatch(albedo_r, albedo_g, albedo_b, 40));
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
            this, [this](int idx) {
                datasetPanel->showImage(idx);
                pixelInfoCard->showWindowLight(idx);
            });
    connect(resultsPanel->diagnosticPanel, &DiagnosticPanel::lightSelected,
            this, [this](int idx) {
                datasetPanel->showImage(idx);
                pixelInfoCard->showWindowLight(idx);
            });

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
        const int wsize    = kBrdfWindowSize;
        const int half     = kBrdfWindowHalf;
        const int wcount   = wsize * wsize;
        const int centerIdx = half * wsize + half;  // flat index of center pixel

        // --- Build Pixel structs for every position in the window ---
		std::vector<Pixel> windowPixels(wcount);
        for (int wi = 0; wi < wcount; ++wi) {
            windowPixels[size_t(wi)].x = x - half + (wi % wsize);
            windowPixels[size_t(wi)].y = y - half + (wi / wsize);
            windowPixels[size_t(wi)].resize(size_t(n));
        }

        // --- Read each image once and fill all window pixels ---
        std::vector<std::vector<Eigen::Vector3f>> perLightColors(n);
        for (int j = 0; j < n; j++) {
            if (cancel->load()) return;
            QImage img = imageset.readImageCropped(size_t(j));
            perLightColors[j].resize(wcount);
            for (int wi = 0; wi < wcount; ++wi) {
                QRgb rgb = img.pixel(windowPixels[size_t(wi)].x,
                                     windowPixels[size_t(wi)].y);
                float r = qRed(rgb)   / 255.0f;
                float g = qGreen(rgb) / 255.0f;
                float b = qBlue(rgb)  / 255.0f;
                windowPixels[size_t(wi)][size_t(j)] = Color3f(r, g, b);
                perLightColors[j][wi] = Eigen::Vector3f(r, g, b);
            }
        }
        QMetaObject::invokeMethod(card, [card, perLightColors]{
            card->setWindowData(perLightColors);
        }, Qt::QueuedConnection);
        if (cancel->load()) return;

        // --- Joint patch normal initializer (shared GGX material) ---
        brdf::PatchInitResult patchInit =
			brdf::init_normal_patch(windowPixels, std::vector<Eigen::Vector3f>(lights.begin(), lights.end()));

        // Use the center-pixel seed result to drive the diagnostic/sphere UI
        const brdf::InitNormalResult& init = patchInit.success
            ? patchInit.seed_results[size_t(centerIdx)]
            : brdf::init_normal(windowPixels[size_t(centerIdx)], lights);
        const Eigen::Vector3f centerNormal = patchInit.success
            ? patchInit.normals[size_t(centerIdx)]
            : init.normal;

        QMetaObject::invokeMethod(card, [card, sphere, diagPanel, rusinView, init, centerNormal]{
            card->setInitResults(
                init.is_metallic, init.highlight_fraction, init.peak_ratio,
                init.lambertian_variance,
                centerNormal.x(), centerNormal.y(), centerNormal.z());
            if (sphere)
                sphere->setNormal(centerNormal);
            if (diagPanel)
                diagPanel->setNormal(centerNormal);
            if (rusinView)
                rusinView->setNormal(centerNormal);
        }, Qt::QueuedConnection);

        // --- Fit every pixel in the window ---
        // Pre-build the normals vector used for each window pixel so we can
        // pass them to optimize_brdf_patch_material after the per-pixel loop.
        std::vector<Eigen::Vector3f> patchNormals(wcount);
        for (int wi = 0; wi < wcount; ++wi)
            patchNormals[wi] = patchInit.success
                ? patchInit.normals[size_t(wi)]
                : patchInit.seed_results[size_t(wi)].normal;

        // Center-pixel albedo seed for the shared material fit
        Eigen::Vector3f centerAlbedo(0.1f, 0.1f, 0.1f);
        {
            Pixel &cp = windowPixels[size_t(centerIdx)];
            for (int k = 0; k < 3; k++) {
                std::vector<float> ch(n);
                for (int j = 0; j < n; j++) ch[j] = cp[j][k];
                std::nth_element(ch.begin(), ch.begin() + n / 2, ch.end());
                centerAlbedo[k] = std::max(ch[n / 2], 0.001f);
            }
        }
        float centerMetallic = (patchInit.seed_results.size() > size_t(centerIdx))
            ? (patchInit.seed_results[size_t(centerIdx)].is_metallic ? 0.5f : 0.0f)
            : (init.is_metallic ? 0.5f : 0.0f);

        for (int wi = 0; wi < wcount; ++wi) {
            if (cancel->load()) return;
            Pixel &wp = windowPixels[size_t(wi)];

            // Albedo estimate: median per channel across lights
            Eigen::Vector3f walbedo = {0.1f, 0.1f, 0.1f};
            for (int k = 0; k < 3; k++) {
                std::vector<float> ch(n);
                for (int j = 0; j < n; j++) ch[size_t(j)] = wp[size_t(j)][k];
                std::nth_element(ch.begin(), ch.begin() + n / 2, ch.end());
                walbedo[k] = std::max(ch[size_t(n / 2)], 0.001f);
            }

            // Use patch-refined normal as warm-start (falls back to seed if patch failed)
            Eigen::Vector3f winormal = patchInit.success
                ? patchInit.normals[size_t(wi)]
                : patchInit.seed_results[size_t(wi)].normal;
            const brdf::InitNormalResult& wseed = patchInit.seed_results.size() > size_t(wi)
                ? patchInit.seed_results[size_t(wi)]
                : init;
            float winit_metallic = wseed.is_metallic ? 0.5f : 0.0f;

            brdf::BrdfFitResult wfit = brdf::optimize_brdf_pixel(
                wp, lights,
                winormal, walbedo,
                0.3f, winit_metallic,
                4.0f, /*optimize_normal=*/false, /*optimize_albedo=*/true);

            // Update the grid cell for this window pixel
            QMetaObject::invokeMethod(card, [card, wi, wfit]{
                card->setFitResults(
                    wi,
                    wfit.normal.x(), wfit.normal.y(), wfit.normal.z(),
                    wfit.roughness, wfit.metallic,
                    wfit.albedo.x(), wfit.albedo.y(), wfit.albedo.z());
            }, Qt::QueuedConnection);
        }

        // --- Shared material fit across the whole patch ---
        if (!cancel->load()) {
            brdf::BrdfFitResult patchFit = brdf::optimize_brdf_patch_material(
                windowPixels, patchNormals, lights,
                centerAlbedo, 0.3f, centerMetallic, 4.0f);
            QMetaObject::invokeMethod(card, [card, patchFit]{
                card->setPatchMaterialResult(
                    patchFit.roughness, patchFit.metallic,
                    patchFit.albedo.x(), patchFit.albedo.y(), patchFit.albedo.z());
            }, Qt::QueuedConnection);
        }
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
