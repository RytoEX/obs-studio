#include "obs.hpp"
#include "scene-tree.hpp"
#include "obs-app.hpp"

#include <QSizePolicy>
#include <QScrollBar>
#include <QDropEvent>
#include <QPushButton>
#include <QTimer>

SceneTree::SceneTree(QWidget *parent_) : QListWidget(parent_)
{
	installEventFilter(this);
	setDragDropMode(InternalMove);
	setMovement(QListView::Snap);
}

void SceneTree::SetGridMode(bool grid)
{
	config_set_bool(App()->GlobalConfig(), "BasicWindow", "gridMode", grid);
	parent()->setProperty("gridMode", grid);
	gridMode = grid;

	if (gridMode) {
		setResizeMode(QListView::Adjust);
		setViewMode(QListView::IconMode);
		setUniformItemSizes(true);
		setStyleSheet("*{padding: 0; margin: 0;}");
	} else {
		setViewMode(QListView::ListMode);
		setResizeMode(QListView::Fixed);
		setStyleSheet("");
	}

	QResizeEvent event(size(), size());
	resizeEvent(&event);
}

bool SceneTree::GetGridMode()
{
	return gridMode;
}

void SceneTree::SetGridItemWidth(int width)
{
	maxWidth = width;
}

void SceneTree::SetGridItemHeight(int height)
{
	itemHeight = height;
}

int SceneTree::GetGridItemWidth()
{
	return maxWidth;
}

int SceneTree::GetGridItemHeight()
{
	return itemHeight;
}

bool SceneTree::eventFilter(QObject *obj, QEvent *event)
{
	QString name = obj->objectName();
	const char *cName = name.toStdString().c_str();
	QEvent::Type eType = event->type();
	if (event->isSinglePointEvent()) {
		//blog(LOG_INFO, "qobj: %s", cName);
		//blog(LOG_INFO, "qeType: %d", eType);
	}
	return QObject::eventFilter(obj, event);
}

void SceneTree::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	if (previous.isValid() && !current.isValid()) {
		blog(LOG_INFO, "no new item selected");
	}
	const int prevIndex = previous.row();
	const int currIndex = current.row();
	blog(LOG_INFO, "prevIndex: %d", prevIndex);
	blog(LOG_INFO, "currIndex: %d", currIndex);
}

void SceneTree::mousePressEvent(QMouseEvent *event)
{
	QEvent::Type eType = event->type();
	blog(LOG_INFO, "qeType: %d", eType);

	//const QList<QListWidgetItem *> items = selectedItems();
	//const qsizetype numItems = items.count();
	const QListWidgetItem *item = currentItem();

	//blog(LOG_INFO, "numItems: %d", numItems);
	if (item) {
		//return;
	}

	QListWidget::mousePressEvent(event);
}

void SceneTree::resizeEvent(QResizeEvent *event)
{
	if (gridMode) {
		int scrollWid = verticalScrollBar()->sizeHint().width();
		const int item_count = count(); // # of items in QListWidget
		const int height_ = height(); // height of widget excluding any window frame
		// QListWidgetItem QListWidget::item(int row)
		// selects row-th item
		// QRect QListWidget::visualItemRect(const QListWidgetItem *item)
		// Returns the rectangle on the viewport occupied by the item at item.
		// int QRect::bottom()
		// Returns the y-coordinate of the rectangle's bottom edge.
		// Note that for historical reasons this function returns top() + height() - 1;
		// use y() + height() to retrieve the true y-coordinate.
		// WAIT.
		// visualItemRect
		// Returns the rectangle on THE VIEWPORT occupied by the item at item.
		// what if it's not visible?
		//
		// the viewport's origin changes as you scroll
		//
		// visualItemRect returns the rectangle relative to current scroll position of the
		// currently visible view of the QListWidget
		// the further "out of view" the last QListWidgetItem is, the larger the value of h.
		// the closer the last QListWidgetItem is to the top of the view of the QListWidget, the smaller the value of h.
		//
		// h is supposed to be the y coordinate of the bottom of the last item in the QListWidget
		// this is used to check "is the bottom of the last widget below the height of the enclosing frame/widget"
		// however, if scrolled to the bottom, this incorrectly asserts that
		// "the bottom of the last widget is above the height of the enclosing frame/widget", so scrollbars are disabled
		// this might be resolved by using y() + height() instead of bottom() ?
		//
		// when scrolled all the way to the bottom, the h < height() is true by 1 pixel
		// thus, y() + height(), which should be 1 pixel larger than bottom(), should resolve this
		const QRect lastitem = visualItemRect(item(count() - 1));
		//int h = visualItemRect(item(count() - 1)).bottom();
		int h = lastitem.y() + lastitem.height();
		int lastY = lastitem.y();
		int lastHeight = lastitem.height();
		int h2 = lastY + lastHeight;
		/*
		blog(LOG_INFO, "resize h: %d", h);
		blog(LOG_INFO, "resize height: %d", height_);
		blog(LOG_INFO, "resize lastY: %d", lastY);
		blog(LOG_INFO, "resize lastHeight: %d", lastHeight);
		blog(LOG_INFO, "resize h2: %d", h2);
		*/

		if (h < height()) {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			scrollWid = 0;
		} else {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		}

		int wid = contentsRect().width() - scrollWid - 1;
		int items = (int)ceil((float)wid / maxWidth);
		int itemWidth = wid / items;

		setGridSize(QSize(itemWidth, itemHeight));

		for (int i = 0; i < count(); i++) {
			item(i)->setSizeHint(QSize(itemWidth, itemHeight));
		}
	} else {
		setGridSize(QSize());
		setSpacing(0);
		for (int i = 0; i < count(); i++) {
			item(i)->setData(Qt::SizeHintRole, QVariant());
		}
	}

	QListWidget::resizeEvent(event);
}

void SceneTree::startDrag(Qt::DropActions supportedActions)
{
	QListWidget::startDrag(supportedActions);
}

void SceneTree::dropEvent(QDropEvent *event)
{
	if (event->source() != this) {
		QListWidget::dropEvent(event);
		return;
	}

	if (gridMode) {
		int scrollWid = verticalScrollBar()->sizeHint().width();
		const QRect firstItem = visualItemRect(item(0));
		const QRect lastItem = visualItemRect(item(count() - 1));
		//int h = visualItemRect(item(count() - 1)).bottom();
		int h = lastItem.y() + lastItem.height();
		const int firstItemY = abs(firstItem.y());

		if (h < height()) {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			scrollWid = 0;
		} else {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		}

		float wid = contentsRect().width() - scrollWid - 1;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QPoint point = event->position().toPoint();
#else
		QPoint point = event->pos();
#endif

		int x = (float)point.x() / wid * ceil(wid / maxWidth);
		int y = (point.y() + firstItemY) / itemHeight;

		int r = x + y * ceil(wid / maxWidth);

		QListWidgetItem *item = takeItem(selectedIndexes()[0].row());
		insertItem(r, item);
		setCurrentItem(item);
		resize(size());
	}

	QListWidget::dropEvent(event);
	//RepositionGrid(); // doesn't work
	//resize(size()); // doesn't work
	// works, but causes new and exciting quirks with scrollbars and scrolling
	QResizeEvent resEvent(size(), size());
	SceneTree::resizeEvent(&resEvent);

	QTimer::singleShot(100, [this]() { emit scenesReordered(); });
}

void SceneTree::RepositionGrid(QDragMoveEvent *event)
{
	int scrollWid = verticalScrollBar()->sizeHint().width();
	const QRect firstItem = visualItemRect(item(0));
	const QRect lastItem = visualItemRect(item(count() - 1));
	//int h = visualItemRect(item(count() - 1)).bottom();
	int h = lastItem.y() + lastItem.height();
	const int firstItemY = abs(firstItem.y());
	const int height_ = height();
	blog(LOG_INFO, "repgrid h: %d", h);
	blog(LOG_INFO, "repgrid height: %d", height_);
	blog(LOG_INFO, "repgrid firstItemY: %d", firstItemY);

	if (h < height()) {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scrollWid = 0;
	} else {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}

	float wid = contentsRect().width() - scrollWid - 1;

	if (event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QPoint point = event->position().toPoint();
#else
		QPoint point = event->pos();
#endif

		// x/y coords are relative to viewport's scrolled origin, not widget's
		// true origin
		// x is probably always okay
		// y needs to be adjusted for scrolling
		int x = (float)point.x() / wid * ceil(wid / maxWidth);
		int y = (point.y() + firstItemY) / itemHeight;

		int r = x + y * ceil(wid / maxWidth);
		int orig = selectedIndexes()[0].row();

		blog(LOG_INFO, "repgrid x: %d", x);
		blog(LOG_INFO, "repgrid y: %d", y);
		blog(LOG_INFO, "repgrid r: %d", r);
		blog(LOG_INFO, "repgrid o: %d", orig);

		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);
			blog(LOG_INFO, "repgrid i: %d", i);

			if (wItem->isSelected())
				continue;

			QModelIndex index = indexFromItem(wItem);

			int off = (i >= r ? 1 : 0) -
				  (i > orig && i > r ? 1 : 0) -
				  (i > orig && i == r ? 2 : 0);

			int xPos = (i + off) % (int)ceil(wid / maxWidth);
			int yPos = (i + off) / (int)ceil(wid / maxWidth);
			QSize g = gridSize(); // gridSize() is the size of the grid spacing (cell size)
			int gw = g.width();
			int gh = g.height();

			//*
			//blog(LOG_INFO, "repgrid i: %d", i);
			blog(LOG_INFO, "visrectbottom: %d", h);
			blog(LOG_INFO, "offset: %d", off);
			blog(LOG_INFO, "xPos: %d", xPos);
			blog(LOG_INFO, "yPos: %d", yPos);
			blog(LOG_INFO, "gw: %d", gw);
			blog(LOG_INFO, "gh: %d", gh);
			//*/

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	} else {
		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected())
				continue;

			QModelIndex index = indexFromItem(wItem);

			int xPos = i % (int)ceil(wid / maxWidth);
			int yPos = i / (int)ceil(wid / maxWidth);
			QSize g = gridSize();

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	}
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	if (gridMode) {
		blog(LOG_INFO, "SceneTree dragMoveEvent gridMode");
		RepositionGrid(event);
	}

	QListWidget::dragMoveEvent(event);
}

void SceneTree::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (gridMode) {
		RepositionGrid();
	}

	QListWidget::dragLeaveEvent(event);
}

void SceneTree::rowsInserted(const QModelIndex &parent, int start, int end)
{
	QResizeEvent event(size(), size());
	SceneTree::resizeEvent(&event);

	QListWidget::rowsInserted(parent, start, end);
}
