#include "imagelist.h"
#include "relightapp.h"

#include <QFileInfo>
#include <QStyledItemDelegate>
#include <assert.h>

class RightDelegate : public QStyledItemDelegate {
public:
	explicit RightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
		QStyleOptionViewItem myOpt(option);
		myOpt.decorationPosition = QStyleOptionViewItem::Right;
		QStyledItemDelegate::paint(painter, myOpt, index);
	}
};


void ImageList::init() {
	setItemDelegate(new RightDelegate(this));
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	Project &project = qRelightApp->project();
	clear();
	int count =0;
	setIconSize(QSize(icon_size, icon_size));

	for(Image &img: project.images) {
		QFileInfo info(img.filename);
		QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(count+1).arg(info.fileName()));
		item->setData(Qt::UserRole, count);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		item->setCheckState(img.skip? Qt::Unchecked : Qt::Checked);
		item->setIcon(QIcon::fromTheme(img.visible ? "eye" : "eye-off"));
		addItem(item);
		count++;
	}

	connect(this, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(verifyItem(QListWidgetItem *)));
}

void ImageList::verifyItem(QListWidgetItem *item) {
	int img_number = item->data(Qt::UserRole).toInt();
	bool skip = item->checkState() != Qt::Checked;

	Project &project = qRelightApp->project();
	assert(img_number >= 0 && size_t(img_number) < project.images.size());

	project.images[img_number].skip = skip;

	emit skipChanged(img_number, skip);
}

void ImageList::setSkipped(int img_number, bool skip) {
	QListWidgetItem *item = this->item(img_number);
	auto &images = qRelightApp->project().images;
	assert(img_number >= 0 && img_number < images.size());
	Image &img = images[img_number];

	item->setCheckState(img.skip? Qt::Unchecked : Qt::Checked);
	item->setIcon(QIcon::fromTheme(img.visible ? "eye" : "eye-off"));
}

void ImageList::mousePressEvent(QMouseEvent *event) {
	QListWidgetItem *item = itemAt(event->pos());
	if (item) {

		QRect itemRect = visualItemRect(item);

		// Setup QStyleOptionViewItem
		QStyleOptionViewItem option;
		option.initFrom(this);
		option.state = QStyle::State_Enabled;
		option.viewItemPosition = QStyleOptionViewItem::Middle;
		option.features = QStyleOptionViewItem::HasCheckIndicator;
		option.rect = itemRect;

		// 1. Get checkbox rect using QStyleOptionViewItem
		QRect checkboxRect = style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, this);

		checkboxRect.moveTopLeft(itemRect.topLeft());
		QRect iconRect(itemRect.right() - 20, itemRect.center().y() - 8, icon_size, 16);

		if (iconRect.contains(event->pos())) {


				int img_number = item->data(Qt::UserRole).toInt();
				auto &images = qRelightApp->project().images;
				assert(img_number >= 0 && img_number < images.size());
				Image &img = images[img_number];
				img.visible = !img.visible;

				item->setIcon(QIcon::fromTheme(img.visible ? "eye" : "eye-off"));
				emit skipChanged(img_number, img.skip);
			}
		}
		QListWidget::mousePressEvent(event);
	}
