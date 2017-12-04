#pragma once

#include <QTreeWidget>

class QMouseEvent;

class SourceTreeWidget : public QTreeWidget {
	Q_OBJECT

	bool ignoreReorder = false;
	int currentRowIndex = -1;
	int topLevelItemsCount = -1;
	int totalItemCount = -1;
public:
	inline SourceTreeWidget(QWidget *parent = nullptr)
		: QTreeWidget(parent)
	{
	}

	int itemCount = 0; // inc/dec on source add/remove

	bool IgnoreReorder() const { return ignoreReorder; }

	// re-implement QListWidget functions to ease the pain of refactoring
	// will probably have to rewrite to find children instead of just top level items
	//void	addItem(const QString &label)
	void addItem(const QString &label)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem;
		item->setText(0, label);
		return addTopLevelItem(item);
	}
	/*int count() const
	{
		return itemCount;
	}*/
	//int count() const { return topLevelItemCount(); } // should return itemCount, or recursively count children
	int currentRow() const
	{
		QTreeWidgetItem *item = currentItem();
		QModelIndex index = indexFromItem(item, 0);
		return index.row();
	}
	QTreeWidgetItem *item(int row)
	{
		return topLevelItem(row);
	}
	void insertItem(int row, QTreeWidgetItem *item)
	{
		// inc itemCount here?
		return insertTopLevelItem(row, item);
	}
	void removeItemWidget(QTreeWidgetItem *item)
	{
		// don't dec itemCount here, it doesn't seem to get called on removal
		//void removeItemWidget(QTreeWidgetItem item, int column);
		return QTreeWidget::removeItemWidget(item, 0);
	}
	void setCurrentRow(int row)
	{
		/*
		QTreeWidgetItem *item = topLevelItem(row);
		return setCurrentItem(item, 0);
		*/
		currentRowIndex = row;
		QTreeWidgetItem *item = topLevelItem(row);
		return setCurrentItem(item, 0);
	}
	void setCurrentRow(int row, QItemSelectionModel::SelectionFlags command)
	{
		//setCurrentItem(QTreeWidgetItem *item, int column, QItemSelectionModel::SelectionFlags command)
		/*
		QTreeWidgetItem *item = topLevelItem(row);
		return setCurrentItem(item, 0, command);
		*/
		currentRowIndex = row;
		QTreeWidgetItem *item = topLevelItem(row);
		return setCurrentItem(item, 0, command);
	}
	QTreeWidgetItem *takeItem(int row)
	{
		return takeTopLevelItem(row);
	}

	// force a recount of all items
	// int recount() vs. int count()
	int count() const
	{
		int topLevelItems = topLevelItemCount();
		int totalItems = topLevelItems;
		QTreeWidgetItem *tmpItem;
		for (int i = 0; i < topLevelItems; i++) {
			tmpItem = topLevelItem(i);
			if (tmpItem)
				totalItems += tmpItem->childCount();
		}
		return totalItems;
	}


protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
};
