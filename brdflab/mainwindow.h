#ifndef BRDFLAB_MAINWINDOW_H
#define BRDFLAB_MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QFutureWatcher>
#include <atomic>

class QSplitter;
class QLabel;
class QSlider;
class QGroupBox;
class QScrollArea;
class QTabWidget;
class QAction;

// Forward declarations for panels (implemented in later phases)
class DatasetPanel;
class ResultsPanel;
class ParamsPanel;

// ---- PixelInfoCard -----------------------------------------------------------
// Compact card shown in the right panel as soon as a pixel is clicked.
// Top half shows init_normal diagnostics; bottom half the Ceres fit results.
// Both halves update independently so the user sees the heuristic estimate
// immediately, then the optimised values once Ceres finishes.
class PixelInfoCard : public QWidget {
	Q_OBJECT
public:
	explicit PixelInfoCard(QWidget *parent = nullptr);

	void setCoords(int x, int y);
	void clearInitResults();
	void clearFitResults();

	// Called after init_normal() completes (fast)
	void setInitResults(bool is_metallic, float highlight_fraction,
	                    float peak_ratio, float lambertian_variance,
	                    float nx, float ny, float nz);

	// Called after Ceres optimization completes (slow)
	void setFitResults(float nx, float ny, float nz,
	                   float roughness, float metallic,
	                   float albedo_r, float albedo_g, float albedo_b);

private:
	QLabel *coords_label;

	// Init results
	QLabel *init_metallic_label;
	QLabel *init_hf_label;
	QLabel *init_pr_label;
	QLabel *init_var_label;
	QLabel *init_normal_swatch; // colour square encoded as normal direction

	// Fit results
	QLabel *fit_normal_swatch;
	QLabel *fit_roughness_label;
	QLabel *fit_metallic_label;
	QLabel *fit_albedo_swatch;

	static QPixmap normalSwatch(float nx, float ny, float nz, int size = 40);
	static QPixmap colorSwatch(float r, float g, float b, int size = 40);
};

// ---- MainWindow --------------------------------------------------------------
class MainWindow : public QMainWindow {
	Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void openFolder();
	void onPixelClicked(int x, int y);

private:
	void createActions();
	void createToolBar();
	void createMenuBar();
	void createStatusBar();
	void createPanels();

	void saveLayout();
	void restoreLayout();

	// ---- Panels ----
	QSplitter   *splitter      = nullptr;
	DatasetPanel *datasetPanel  = nullptr;  // left
	ResultsPanel *resultsPanel  = nullptr;  // centre
	QWidget      *rightPanel    = nullptr;  // right (params + info card)
	ParamsPanel  *paramsPanel   = nullptr;
	PixelInfoCard *pixelInfoCard = nullptr;

	// ---- Status bar widgets ----
	QLabel *statusFolder = nullptr;
	QLabel *statusPixel  = nullptr;
	QLabel *statusState  = nullptr;

	// ---- Actions ----
	QAction *actOpen   = nullptr;
	QAction *actPrev   = nullptr;
	QAction *actNext   = nullptr;
	QAction *actFit    = nullptr;
	QAction *actRecompute = nullptr;

	QSettings settings;

	// ---- Async pixel computation ----
	int clickedX = -1;
	int clickedY = -1;
	QFutureWatcher<void> *computeWatcher = nullptr;
	std::atomic<bool> cancelCompute{false};
};

#endif // BRDFLAB_MAINWINDOW_H
