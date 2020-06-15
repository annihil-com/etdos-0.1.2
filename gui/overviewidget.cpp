#include "overviewidget.h"
#include "slave.h"
#include "mainwindow.h"

#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabBar>
#include <QPainter>
#include <QTextDocument>
#include <etdos/protocol.h>

OverViewWidget::OverViewWidget(QWidget *parent) : QWidget(parent)
{
	model = 0;

	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	slaveGroupBox = new QGroupBox("Slaves", this);

    slaveView = new SlaveView;
    slaveView->setRootIsDecorated(false);
    slaveView->setAlternatingRowColors(true);
	createSlaveModel();

    QHBoxLayout *slaveboxLayout = new QHBoxLayout;
    slaveboxLayout->addWidget(slaveView);
    slaveGroupBox->setLayout(slaveboxLayout);

	mainLayout->addWidget(slaveGroupBox);

	resize(500, 450);
}

void OverViewWidget::slotSlaveChange(SlaveClient *sl)
{
	int c = model->rowCount();
	for (int i = 0; i < c; i++) {
		QStandardItem *entry = model->item(i, 0);
		if (entry && entry->text().toInt() == sl->Id()) {
			model->setData(model->index(i, 0), sl->Id());
			model->setData(model->index(i, 1), sl->Name());
			model->setData(model->index(i, 2), sl->Server());
			model->setData(model->index(i, 3), sl->Status());
			model->setData(model->index(i, 4), sl->ClientNum());
			model->setData(model->index(i, 5), sl->Role());
		}
	}
}


void OverViewWidget::slotSlaveSpawn(SlaveClient *sl)
{
	int cnt = model->rowCount();
	model->insertRow(cnt);
	model->setData(model->index(cnt, 0), sl->Id());
	model->setData(model->index(cnt, 1), sl->Name());
	model->setData(model->index(cnt, 2), sl->Server());
	model->setData(model->index(cnt, 3), sl->Status());
	model->setData(model->index(cnt, 4), sl->ClientNum());
	model->setData(model->index(cnt, 5), sl->Role());
}

void OverViewWidget::slotSlaveExit(SlaveClient *sl)
{
	int c = model->rowCount();
	for (int i = 0; i < c; i++) {
		QStandardItem *entry = model->item(i, 0);
		if (entry && entry->text().toInt() == sl->Id()) {
			model->removeRow(i);
		}
	}
}

QList<int>* OverViewWidget::SlavesSelected()
{
	QModelIndexList sl = selectionModel->selectedRows(0);
	QList<int>* IdList = new QList<int>;

	int sls = sl.size();
	for (int i=0; i<sls; i++)
		IdList->push_back(sl.at(i).data().toInt());

	return IdList;
}

void OverViewWidget::createSlaveModel()
{
	model = new QStandardItemModel(0, 6, this);
	model->setHeaderData(0, Qt::Horizontal, QObject::tr("pID"));
	model->setHeaderData(1, Qt::Horizontal, QObject::tr("name"));
	model->setHeaderData(2, Qt::Horizontal, QObject::tr("server"));
	model->setHeaderData(3, Qt::Horizontal, QObject::tr("status"));
	model->setHeaderData(4, Qt::Horizontal, QObject::tr("clnum"));
	slaveView->setModel(model);

	selectionModel = new QItemSelectionModel(model);
	slaveView->setSelectionModel(selectionModel);
	slaveView->setColumnHidden(5, true);
}

// reimplement treeview to customize the way our items are drawn
// with Q3 color codes actually colored ;)
SlaveView::SlaveView(QWidget *parent) : QTreeView(parent)
{
	setUniformRowHeights(true);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::MultiSelection);
}

void SlaveView::drawSlaveName(QPainter *painter, QRect &rect, QModelIndex &s)
{
	QBrush bg(Qt::SolidPattern);
	bg.setColor(QColor(160,160,160));
	painter->fillRect(rect, bg);
	QTextDocument richText;
	QString htmlCode = QString("%1%2%3")
			.arg("<font size=\"2\">")
			.arg(MainWindow::Q3ColorStringtoHtml(s.data().toString()))
			.arg("</font>");

	richText.setHtml(htmlCode);

	painter->translate(QPointF(rect.x(),rect.y()-2.));
	const QRectF vp(0, 0, rect.width(), rect.height());
	richText.drawContents(painter, vp);
	painter->translate(-QPointF(rect.x(),rect.y()-2.));
}

void SlaveView::drawRow(QPainter *painter, const QStyleOptionViewItem &z, const QModelIndex &index) const
{
	QModelIndexList sl = selectionModel()->selectedRows();

	QFont f(painter->font());
	f.setPointSize(7);
	painter->setFont(f);

	if (sl.contains(index)) {
		QTreeView::drawRow(painter,z,index);
		SlaveView* ptr = const_cast<SlaveView*>(this);
		QModelIndex s = index.sibling(index.row(), 1);
		QRect rect = visualRect(s);
		ptr->drawSlaveName(painter, rect, s);
		return;
	}

	for (int c=0; c<5; c++) {
		QModelIndex s = index.sibling(index.row(), c);

		if (s.isValid()) {
			QRect rect = visualRect(s);
			QBrush bg(Qt::SolidPattern);

			switch (c)
			{
				default:
				{
					bg.setColor(QColor(174,216,255));
					painter->fillRect(rect, bg);
					painter->translate(4., 0.);
					painter->drawText(QRectF(rect), s.data().toString());
					painter->translate(-4., 0.);
					break;
				}
				case 0:
				{
					if (index.sibling(index.row(), 5).data().toInt() == ROLE_SCOUT)
						bg.setColor(QColor(255,165,165));
					else
						bg.setColor(QColor(174,216,255));

					painter->fillRect(rect, bg);
					painter->translate(4., 0.);
					painter->drawText(QRectF(rect), s.data().toString());
					painter->translate(-4., 0.);
					break;
				}
				case 1:
				{
					SlaveView* ptr = const_cast<SlaveView*>(this);
					ptr->drawSlaveName(painter, rect, s);
					break;
				}
			}
		}
	}
}
