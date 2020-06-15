/*
	OverView Widget displays info about all
	spawned bots, and allows you to interact
	with them
*/

#ifndef __OVERVIEW_WID_H_
#define __OVERVIEW_WID_H_

#include <QWidget>
#include <QTreeView>

class QItemSelectionModel;
class QStandardItemModel;
class QGroupBox;
class QLabel;
class QTreeView;
class QTabBar;
class SlaveClient;
class SlaveView;
class QPainter;
class QRect;

class OverViewWidget : public QWidget
{
	Q_OBJECT

public:
	OverViewWidget(QWidget *parent = 0);
	QList<int>* SlavesSelected();

private slots:
	void slotSlaveChange(SlaveClient *sl);
	void slotSlaveSpawn(SlaveClient *sl);
	void slotSlaveExit(SlaveClient *sl);

private:
	QStandardItemModel *model;
	QItemSelectionModel *selectionModel;
	QTabBar		*tabbar;
	QGroupBox 	*slaveGroupBox;
	SlaveView 	*slaveView;

	void createSlaveModel();
};

class SlaveView : public QTreeView
{
	Q_OBJECT

	public:
		SlaveView(QWidget * parent = 0);

	protected:
		void drawRow(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

	private:
		void drawSlaveName(QPainter *painter, QRect &rect, QModelIndex &s);
};

#endif
