#include <QMouseEvent>

#include <vector>

#include "qt-wrappers.hpp"
#include "source-tree-widget.hpp"

Q_DECLARE_METATYPE(OBSSceneItem);

void SourceTreeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		QTreeWidget::mouseDoubleClickEvent(event);
}

/* dragDropMode = QAbstractItemView::InternalMove
 * defaultDropAction = Qt::TargetMoveAction
 * only triggered when reordering internally
 */
void SourceTreeWidget::dropEvent(QDropEvent *event)
{
	QTreeWidget::dropEvent(event);
	if (!event->isAccepted() || !count())
		return;

	QPoint dropPos = event->pos();
	QTreeWidgetItem *dropTargetItem = itemAt(dropPos);
	if (dropTargetItem) {
		QString dropTargetName = dropTargetItem->text(0);
		std::string dtName = dropTargetName.toStdString();
		int numChildren = dropTargetItem->childCount();
	}

	auto GetSceneItem = [&](int i)
	{
		return item(i)->data(0, Qt::UserRole).value<OBSSceneItem>();
	};

	std::vector<obs_sceneitem_t*> newOrder;
	newOrder.reserve(count());
	for (int i = count() - 1; i >= 0; i--)
		newOrder.push_back(GetSceneItem(i));

	auto UpdateOrderAtomically = [&](obs_scene_t *scene)
	{
		ignoreReorder = true;
		obs_scene_reorder_items(scene, newOrder.data(),
				newOrder.size());
		ignoreReorder = false;
	};
	using UpdateOrderAtomically_t = decltype(UpdateOrderAtomically);

	auto scene = obs_sceneitem_get_scene(GetSceneItem(0));
	obs_scene_atomic_update(scene, [](void *data, obs_scene_t *scene)
	{
		(*static_cast<UpdateOrderAtomically_t*>(data))(scene);
	}, static_cast<void*>(&UpdateOrderAtomically));
}
