#include "sharpenframe.h"
#include "imageview.h"
#include "relightapp.h"
#include "helpbutton.h"

#include <QGraphicsPixmapItem>
#include <QByteArray>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QSlider>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>

SharpenFrame::SharpenFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->addWidget(new QLabel("<h2>Unsharp mask</h2>", this));
	layout->addSpacing(20);

	navigation_toolbar = new QToolBar("Image navigation", this);
	navigation_toolbar->setMovable(false);
	layout->addWidget(navigation_toolbar, 0, Qt::AlignHCenter);
	QAction *rotate_left = qRelightApp->action("rotate_left");
	QAction *rotate_right = qRelightApp->action("rotate_right");
	QAction *zoom_fit = qRelightApp->action("zoom_fit");
	QAction *zoom_one = qRelightApp->action("zoom_one");
	QAction *zoom_in = qRelightApp->action("zoom_in");
	QAction *zoom_out = qRelightApp->action("zoom_out");
	navigation_toolbar->addAction(rotate_left->icon(), rotate_left->text(), this, &SharpenFrame::rotateLeft);
	navigation_toolbar->addAction(rotate_right->icon(), rotate_right->text(), this, &SharpenFrame::rotateRight);
	navigation_toolbar->addSeparator();
	navigation_toolbar->addAction(zoom_fit->icon(), zoom_fit->text(), this, [this]() { image_view->fit(); });
	navigation_toolbar->addAction(zoom_one->icon(), zoom_one->text(), this, [this]() { image_view->one(); });
	navigation_toolbar->addAction(zoom_in->icon(), zoom_in->text(), this, [this]() { image_view->zoomIn(); });
	navigation_toolbar->addAction(zoom_out->icon(), zoom_out->text(), this, [this]() { image_view->zoomOut(); });

	QWidget *content = new QWidget(this);
	QHBoxLayout *content_layout = new QHBoxLayout(content);
	content_layout->setContentsMargins(0, 0, 0, 0);
	content_layout->setSpacing(8);
	QVBoxLayout *viewer_layout = new QVBoxLayout;
	viewer_layout->setContentsMargins(0, 0, 0, 0);
	viewer_layout->setSpacing(4);
	image_info = new QLabel(content);
	image_info->setFixedHeight(image_info->fontMetrics().height());
	viewer_layout->addWidget(image_info);
	image_view = new ImageView(content);
	image_view->setMinimumSize(320, 240);
	viewer_layout->addWidget(image_view, 1);
	content_layout->addLayout(viewer_layout, 1);

	QWidget *sidebar = new QWidget(content);
	sidebar->setFixedWidth(320);
	QVBoxLayout *sidebar_layout = new QVBoxLayout(sidebar);
	sidebar_layout->setContentsMargins(0, 0, 8, 0);
	sidebar_layout->setSpacing(8);

	auto addLoadButton = [this, sidebar, sidebar_layout, viewer_layout](const QString &text, const QIcon &icon,
												 void (SharpenFrame::*slot)()) {
		HelpedButton *button = new HelpedButton("interface/sharpen", icon, text, sidebar);
		if(sidebar_layout->count() == 0) {
			const int top_offset = image_info->height() + viewer_layout->spacing()
				- button->layout()->contentsMargins().top();
			if(top_offset > 0)
				sidebar_layout->addSpacing(top_offset);
		}
		sidebar_layout->addWidget(button);
		connect(button, &HelpedButton::clicked, this, slot);
	};

	addLoadButton("Load image", QIcon::fromTheme("image-x-generic"), &SharpenFrame::loadImage);
	addLoadButton("Load normal", QIcon::fromTheme("image-x-generic"), &SharpenFrame::loadNormal);
	addLoadButton("Load height", QIcon::fromTheme("image-x-generic"), &SharpenFrame::loadHeight);
	sidebar_layout->addSpacing(8);

	auto addSlider = [this, sidebar_layout, sidebar](const QString &title, int minimum, int maximum, int initial,
						QSlider *&slider, const QString &suffix, int scale = 1) {
		QWidget *control = new QWidget(sidebar);
		QHBoxLayout *control_layout = new QHBoxLayout(control);
		control_layout->setContentsMargins(0, 0, 0, 0);
		control_layout->setSpacing(2);

		QLabel *name = new QLabel(title, control);
		name->setFixedWidth(48);
		slider = new QSlider(Qt::Horizontal, control);
		slider->setRange(minimum, maximum);
		slider->setValue(initial);
		slider->setToolTip(title);
		const int decimals = scale == 1 ? 0 : 1;
		QLabel *value = new QLabel(QString::number(static_cast<double>(initial) / scale, 'f', decimals) + suffix, control);
		value->setFixedWidth(scale == 1 ? 36 : 48);
		connect(slider, &QSlider::valueChanged, value, [value, suffix, scale, decimals](int v) {
			value->setText(QString::number(static_cast<double>(v) / scale, 'f', decimals) + suffix);
		});

		control_layout->addWidget(name);
		control_layout->addWidget(slider, 1);
		control_layout->addWidget(value);
		sidebar_layout->addWidget(control);
	};

	addSlider("Radius", 0, 500, 100, radius_slider, "px", 10);
	addSlider("Gain", 0, 100, 50, gain_slider, "", 100);
	addSlider("Min", 0, 100, 0, minimum_value_slider, "");
	addSlider("Max", 0, 100, 100, maximum_value_slider, "");

	connect(minimum_value_slider, &QSlider::valueChanged, this, [this](int value) {
		if(value > maximum_value_slider->value())
			maximum_value_slider->setValue(value);
	});
	connect(maximum_value_slider, &QSlider::valueChanged, this, [this](int value) {
		if(value < minimum_value_slider->value())
			minimum_value_slider->setValue(value);
	});
	connect(radius_slider, &QSlider::valueChanged, this, [this](int value) {
		sharpen_data.sigma = static_cast<float>(value) / 10.0f;
		updatePreview();
	});
	connect(gain_slider, &QSlider::valueChanged, this, [this](int) { updatePreview(); });
	connect(minimum_value_slider, &QSlider::valueChanged, this, [this](int) { updatePreview(); });
	connect(maximum_value_slider, &QSlider::valueChanged, this, [this](int) { updatePreview(); });

	sidebar_layout->addWidget(new QLabel("False color:", sidebar));
	color_scheme = new QComboBox(sidebar);
	color_scheme->addItems({"Grayscale", "Viridis", "Plasma", "Inferno", "Magma", "Cividis", "Turbo"});
	color_scheme->setToolTip("False-color display scheme");
	sidebar_layout->addWidget(color_scheme);
	connect(color_scheme, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		sharpen_data.heightColorScheme = static_cast<HeightColorScheme>(index);
		updatePreview();
	});
	sidebar_layout->addStretch(1);
	content_layout->addWidget(sidebar);
	layout->addWidget(content, 1);
}

void SharpenFrame::loadImage() {
	loadFile(SharpenInputType::Image, "image");
}

void SharpenFrame::loadNormal() {
	loadFile(SharpenInputType::Normals, "normal map");
}

void SharpenFrame::loadHeight() {
	loadFile(SharpenInputType::Heightmap, "heightmap");
}

void SharpenFrame::loadFile(SharpenInputType inputType, const QString &kind) {
	const QString path = QFileDialog::getOpenFileName(
		this, "Load " + kind, QString(),
		"Images (*.jpg *.JPG *.jpeg *.JPEG *.png *.PNG *.tif *.TIF *.tiff *.TIFF *.exr *.EXR);;All files (*)");
	if(path.isEmpty())
		return;

	const QByteArray encodedPath = QFile::encodeName(path);
	const std::string nativePath(encodedPath.constData(), size_t(encodedPath.size()));
	try {
		switch(inputType) {
		case SharpenInputType::Image:
			sharpen_data.loadImage(nativePath);
			break;
		case SharpenInputType::Normals:
			sharpen_data.loadNormalmap(nativePath);
			break;
		case SharpenInputType::Heightmap:
			sharpen_data.loadHeightmap(nativePath);
			break;
		}
	} catch(const QString &error) {
		QMessageBox::critical(this, "Could not load " + kind, error);
		return;
	}

	image_info->setText(QString("%1 — %2 × %3")
		.arg(QFileInfo(path).fileName())
		.arg(sharpen_data.width)
		.arg(sharpen_data.height));
	rotation = 0;
	updatePreview(true);
}

void SharpenFrame::updatePreview(bool fitView) {
	if(sharpen_data.width == 0 || sharpen_data.height == 0)
		return;

	sharpen_data.sigma = static_cast<float>(radius_slider->value()) / 10.0f;
	sharpen_data.heightColorScheme = static_cast<HeightColorScheme>(color_scheme->currentIndex());
	const float gain = static_cast<float>(gain_slider->value()) / 100.0f;
	QPixmap pixmap;
	switch(sharpen_data.type) {
	case SharpenInputType::Image: {
		std::vector<Eigen::Vector3f> sharpened = unsharpMaskImage(
			sharpen_data.image, sharpen_data.width, sharpen_data.height, sharpen_data.sigma);
		for(size_t i = 0; i < sharpened.size(); ++i)
			sharpened[i] = (1.0f - gain) * sharpen_data.image[i] + gain * sharpened[i];
		pixmap = renderImage(sharpened, sharpen_data.width, sharpen_data.height);
		break;
	}
	case SharpenInputType::Normals: {
		const std::vector<Eigen::Vector2f> detail = unsharpMaskNormals(
			sharpen_data.normals, sharpen_data.width, sharpen_data.height, sharpen_data.sigma);
		std::vector<Eigen::Vector2f> sharpened(detail.size());
		for(size_t i = 0; i < detail.size(); ++i) {
			const Eigen::Vector3f &normal = sharpen_data.normals[i];
			const float z = std::abs(normal.z()) < 1e-6f ? 1e-6f : normal.z();
			const Eigen::Vector2f originalSlope(normal.x() / z, normal.y() / z);
			sharpened[i] = (1.0f - gain) * originalSlope + gain * detail[i];
		}
		pixmap = renderNormal(sharpened, sharpen_data.width, sharpen_data.height, sharpen_data.normalColorScheme);
		break;
	}
	case SharpenInputType::Heightmap: {
		std::vector<float> sharpened = unsharpMaskHeightmap(
			sharpen_data.heightmap, sharpen_data.width, sharpen_data.height, sharpen_data.sigma);
		for(size_t i = 0; i < sharpened.size(); ++i)
			sharpened[i] = (1.0f - gain) * sharpen_data.heightmap[i] + gain * sharpened[i];
		float processedMin = std::numeric_limits<float>::infinity();
		float processedMax = -std::numeric_limits<float>::infinity();
		for(float value: sharpened) {
			if(!std::isfinite(value))
				continue;
			processedMin = std::min(processedMin, value);
			processedMax = std::max(processedMax, value);
		}
		if(!std::isfinite(processedMin) || !std::isfinite(processedMax)) {
			processedMin = 0.0f;
			processedMax = 1.0f;
		}
		const float range = processedMax - processedMin;
		const float minValue = processedMin + range * (minimum_value_slider->value() / 100.0f);
		const float maxValue = processedMin + range * (maximum_value_slider->value() / 100.0f);
		pixmap = renderHeight(sharpened, sharpen_data.width, sharpen_data.height,
			minValue, maxValue, sharpen_data.heightColorScheme);
		break;
	}
	}

	image_view->imagePixmap->setPixmap(pixmap);
	image_view->imagePixmap->setTransform(QTransform());
	image_view->imagePixmap->setTransformOriginPoint(image_view->imagePixmap->boundingRect().center());
	image_view->imagePixmap->setRotation(rotation);
	image_view->scene.setSceneRect(image_view->scene.itemsBoundingRect());
	if(fitView)
		image_view->fit();
}

void SharpenFrame::rotateLeft() {
	rotateImage(-90);
}

void SharpenFrame::rotateRight() {
	rotateImage(90);
}

void SharpenFrame::rotateImage(int degrees) {
	if(image_view->imagePixmap->pixmap().isNull())
		return;
	rotation = (rotation + degrees + 360) % 360;
	image_view->imagePixmap->setRotation(rotation);
	image_view->scene.setSceneRect(image_view->scene.itemsBoundingRect());
	image_view->fit();
}
