/* This file is part of the KDE project
   Copyright (C) 2002 Till Busch <till@bux.at>
   Lucijan Busch <lucijan@gmx.at>
   Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Original Author:  Till Busch <till@bux.at>
   Original Project: buX (www.bux.at)
*/

#include <qpainter.h>
#include <qkeycode.h>
#include <qheader.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qwmatrix.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qstyle.h>

#include <config.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapp.h>
#include <kiconloader.h>

#ifndef KEXI_NO_PRINT
# include <kprinter.h>
# include <kdeprint/driver.h>
#endif

#include "kexitablerm.h"
#include "kexitableview.h"
#include "kexitableheader.h"

#ifdef USE_KDE
#include "kexidatetableedit.h"
#endif

#include "kexitableedit.h"
#include "kexiinputtableedit.h"
#include "kexicomboboxtableedit.h"
#include "kexiblobtableedit.h"

KexiTableView::KexiTableView(QWidget *parent, const char *name, KexiTableList *contents)
:QScrollView(parent, name, Qt::WRepaintNoErase | Qt::WStaticContents | Qt::WResizeNoErase)
	,m_editOnDubleClick(true)
{
	setResizePolicy(Manual);
	viewport()->setBackgroundMode(NoBackground);
	viewport()->setFocusPolicy(QWidget::StrongFocus);
	setBackgroundMode(NoBackground);
	setLineWidth(1);
	// Initialize variables
	m_pEditor = 0;
	m_numRows = 0;
	m_numCols = 0;
	m_curRow = -1;
	m_curCol = -1;
	m_pCurrentItem = 0;
	m_pInsertItem = 0;
	m_pContextMenu = new QPopupMenu(this);
	menu_id_addRecord = m_pContextMenu->insertItem(i18n("Add Record"), this, SLOT(addRecord()), CTRL+Key_Insert);
	menu_id_removeRecord = m_pContextMenu->insertItem(kapp->iconLoader()->loadIcon("button_cancel", KIcon::Small), i18n("Remove Record"), this, SLOT(removeRecord()), CTRL+Key_Delete);

#ifdef Q_WS_WIN
  m_rowHeight = fontMetrics().lineSpacing() + 4;
#else
  m_rowHeight = fontMetrics().lineSpacing();
#endif
	if(m_rowHeight < 17)
		m_rowHeight = 17;
	m_pBufferPm = 0;

	m_sortedColumn = -1;
	m_sortOrder = true;

	m_pUpdateTimer = new QTimer(this);

	m_pColumnTypes = new QMemArray<QVariant::Type>;
	m_pColumnModes = new QMemArray<int>;
	m_pColumnDefaults = new QPtrList<QVariant>;
	m_deletionPolicy = NoDelete;
	//m_additionPolicy = NoAdd;
  setAdditionPolicy( NoAdd );

	setMargins(14, fontMetrics().height() + 4, 0, 0);

	// Create headers
	m_pTopHeader = new QHeader(m_numCols, this);
	m_pTopHeader->setOrientation(Horizontal);
	m_pTopHeader->setTracking(false);
	m_pTopHeader->setMovingEnabled(false);

/*	m_pVerticalHeader = new KexiTableRM(this);
	m_pVerticalHeader->setCellHeight(m_rowHeight);
	m_pVerticalHeader->setCurrentRow(-1);
*/

	m_pVerticalHeader = new KexiTableHeader(this);
	m_pVerticalHeader->setOrientation(Vertical);
	m_pVerticalHeader->setTracking(false);
	m_pVerticalHeader->setMovingEnabled(false);
	m_pVerticalHeader->setStretchEnabled(false);
	m_pVerticalHeader->setResizeEnabled(false);
	m_pVerticalHeader->setCurrentRow(false);

//	enableClipper(true);
	setBackgroundMode(PaletteBackground);

	if(!contents)
	{
		m_contents = new KexiTableList();
	}
	else
	{
		kdDebug() << "KexiTableView::KexiTableView(): using shared contents" << endl;
		m_contents = contents;
		m_numRows = contents->count();
		triggerUpdate();
		KexiTableItem *it;
		for(it = m_contents->first(); it; it = m_contents->next())
		{
			if(!it->isInsertItem())
				m_pVerticalHeader->addLabel("",  m_rowHeight);
			else
				m_pVerticalHeader->addLabel("*",  m_rowHeight);
		}
	}

	m_contents->setAutoDelete(true);

	m_scrollTimer = new QTimer(this);
	connect(m_scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));

	setBackgroundAltering(true);

	// Connect header, table and scrollbars
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), m_pTopHeader, SLOT(setOffset(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),	m_pVerticalHeader, SLOT(setOffset(int)));
	connect(m_pTopHeader, SIGNAL(sizeChange(int, int, int)), this, SLOT(columnWidthChanged(int, int, int)));
	connect(m_pTopHeader, SIGNAL(clicked(int)), this, SLOT(columnSort(int)));

	connect(m_pUpdateTimer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
	m_recordIndicator = false;
}

void KexiTableView::addDropFilter(const QString &filter)
{
	m_dropFilters.append(filter);
}

void KexiTableView::addColumn(QString name, QVariant::Type type, bool editable, QVariant defaultValue,
 int width, bool autoinc)
{
	m_numCols++;
	m_pColumnTypes->resize(m_numCols);
	m_pColumnModes->resize(m_numCols);

	m_pColumnTypes->at(m_numCols-1)		= type;

	if(editable && autoinc)
	{
		m_pColumnModes->at(m_numCols-1) = ColumnEditable | ColumnAutoIncrement;
	}
	else if(editable)
	{
		m_pColumnModes->at(m_numCols-1) = ColumnEditable;
	}
	else
	{
		m_pColumnModes->at(m_numCols-1) = ColumnReadOnly;
	}

	m_pColumnDefaults->append(new QVariant(defaultValue));
	m_pTopHeader->addLabel(name, width);
	m_pTopHeader->setUpdatesEnabled(true);
}


void KexiTableView::setFont(const QFont &f)
{
	QWidget::setFont(f);
#ifdef Q_WS_WIN
  m_rowHeight = fontMetrics().lineSpacing() + 4;
#else
  m_rowHeight = fontMetrics().lineSpacing() + 2;
#endif
	if(m_rowHeight < 22)
		m_rowHeight = 22;
	setMargins(14, m_rowHeight, 0, 0);
	m_pVerticalHeader->setCellHeight(m_rowHeight);
}

void KexiTableView::remove(KexiTableItem *item, bool moveCursor/*=true*/)
{
	if(m_contents->removeRef(item))
	{
		m_pVerticalHeader->removeLabel(m_curRow);
		m_numRows--;
		if(moveCursor)
			setCursor(m_curRow);
//		selectPrev();
		m_pUpdateTimer->start(1,true);
	}
}

void KexiTableView::removeRecord()
{
	if (m_deletionPolicy == NoDelete)
    return;
  if (m_deletionPolicy == ImmediateDelete && m_pCurrentItem) {
    remove(m_pCurrentItem);
  } else if (m_deletionPolicy == AskDelete) {
    //TODO(js)
  } else if (m_deletionPolicy == SignalDelete) {
  	emit itemRemoveRequest(m_pCurrentItem);
	  emit currentItemRemoveRequest();
  }
}

void KexiTableView::addRecord()
{
	emit addRecordRequest();
}

void KexiTableView::clear()
{
	for(int i=0; i < rows(); i++)
	{
		m_pVerticalHeader->removeLabel(i);
	}

	editorCancel();
	m_contents->clear();

	// Initialize variables
	m_pEditor = 0;
	m_numRows = 0;
	m_curRow = -1;
	m_curCol = -1;
	m_pCurrentItem=0;

	m_pVerticalHeader->setCurrentRow(-1);
//	m_pUpdateTimer->start(1,true);
	viewport()->repaint();
}

void KexiTableView::clearAll()
{
	for(int i=0; i < rows(); i++)
	{
		m_pVerticalHeader->removeLabel(i);
	}

	editorCancel();
	m_contents->clear();

	m_pEditor = 0;
	m_numRows = 0;
	m_numCols = 0;
	m_curRow = 0;
	m_curCol = 0;
	m_pCurrentItem=0;

	while(m_pTopHeader->count())
		m_pTopHeader->removeLabel(0);

	m_pVerticalHeader->setCurrentRow(-1);

	viewport()->repaint();

	m_sortedColumn = -1;
	m_sortOrder = true;

	m_pColumnTypes->resize(0);
	m_pColumnModes->resize(0);
	m_pColumnDefaults->clear();
}

int KexiTableView::findString(const QString &string)
{
	int row = 0;
	int col = sorting();
	if(col == -1)
		return -1;
	if(string.isEmpty())
	{
		setCursor(0, col);
		return 0;
	}

	QPtrListIterator<KexiTableItem> it(*m_contents);

	if(string.at(0) != QChar('*'))
	{
		switch(columnType(col))
		{
			case QVariant::String:
			{
				QString str2 = string.lower();
				for(; it.current(); ++it)
				{
					if(it.current()->getText(col).left(string.length()).lower().compare(str2)==0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;
			}
			case QVariant::Int:
			case QVariant::Bool:
				for(; it.current(); ++it)
				{
					if(QString::number(it.current()->getInt(col)).left(string.length()).compare(string)==0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;

			default:
				break;
		}
	}
	else
	{
		QString str2 = string.mid(1);
		switch(columnType(col))
		{
			case QVariant::String:
				for(; it.current(); ++it)
				{
					if(it.current()->getText(col).find(str2,0,false) >= 0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;
			case QVariant::Int:
			case QVariant::Bool:
				for(; it.current(); ++it)
				{
					if(QString::number(it.current()->getInt(col)).find(str2,0,true) >= 0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;

			default:
				break;
		}
	}
	return -1;
}


KexiTableView::~KexiTableView()
{
	editorCancel();

	delete m_pColumnTypes;
	delete m_pColumnModes;
	delete m_pColumnDefaults;
        delete m_pBufferPm;
}
/*
void KexiTableView::setColumn(int col, QString name, ColumnTyFpe type, bool changeable=false)
{
	(m_pColumnTypes->at(col)) = type;
	m_pTopHeader->setLabel(col, name);
	(m_pColumnModes->at(col)) = changeable;
}
*/
void KexiTableView::setSorting(int col, bool ascending/*=true*/)
{
	m_sortOrder = ascending;
	m_sortedColumn = col;
	m_pTopHeader->setSortIndicator(col, m_sortOrder);
	m_contents->setSorting(col, m_sortOrder, columnType(col));
	sort();
}

void KexiTableView::slotUpdate()
{
//	static int count=1;
	// qDebug("%s: slotUpdate() - %04d", name(), count++);
	QSize s(tableSize());
//	viewport()->setUpdatesEnabled(false);
	resizeContents(s.width(), s.height());
//	viewport()->setUpdatesEnabled(true);
	updateContents(0, 0, viewport()->width(), contentsHeight());
	updateGeometries();
}

int KexiTableView::sorting()
{
	return m_sortedColumn;
}

void KexiTableView::sort()
{
	if(rows() < 2)
	{
		return;
	}


	m_contents->sort();

	m_curRow = m_contents->findRef(m_pCurrentItem);

	m_pCurrentItem = m_contents->at(m_curRow);

	int cw = columnWidth(m_curCol);
	int rh = rowHeight(m_curRow);

//	m_pVerticalHeader->setCurrentRow(m_curRow);
	center(columnPos(m_curCol) + cw / 2, rowPos(m_curRow) + rh / 2);
//	updateCell(oldRow, m_curCol);
//	updateCell(m_curRow, m_curCol);
	m_pVerticalHeader->setCurrentRow(m_curRow);
//	slotUpdate();
	m_pUpdateTimer->start(1,true);
}

QSizePolicy KexiTableView::sizePolicy() const
{
	// this widget is expandable
	return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize KexiTableView::sizeHint() const
{
	return QSize(tableSize().width(), m_rowHeight*3);
}

QSize KexiTableView::minimumSizeHint() const
{
	return QSize(columnWidth(1), m_rowHeight*2);
}

void KexiTableView::createBuffer(int width, int height)
{
	if(!m_pBufferPm)
		m_pBufferPm = new QPixmap(width, height);
	else
		if(m_pBufferPm->width() < width || m_pBufferPm->height() < height)
			m_pBufferPm->resize(width, height);
//	m_pBufferPm->fill();
}

void KexiTableView::drawContents( QPainter *p, int cx, int cy, int cw, int ch )
{
	int colfirst = columnAt(cx);
	int rowfirst = rowAt(cy);
	int collast = columnAt(cx + cw-1);
	int rowlast = rowAt(cy + ch-1);

//	qDebug("%3d %3d %3d %3d | %2d, %2d, %2d, %2d [%4d, %4d]", cx, cy, cw, ch,
//													colfirst, rowfirst, collast, rowlast, tableSize().width(), tableSize().height());

	if (rowfirst == -1 || colfirst == -1)
	{
		paintEmptyArea(p, cx, cy, cw, ch);
		return;
	}

	createBuffer(cw, ch);
	if(m_pBufferPm->isNull())
		return;
	QPainter *pb = new QPainter(m_pBufferPm, this);

	if (rowlast == -1)
		rowlast = rows() - 1;
	if ( collast == -1 )
		collast = cols() - 1;

	int r;
	pb->fillRect(0, 0, cw, ch, colorGroup().base());

	QPtrListIterator<KexiTableItem> it(*m_contents);
	it += rowfirst;

	int maxwc = QMIN(cw, (columnPos(m_numCols - 1) + columnWidth(m_numCols - 1)));
	pb->fillRect(maxwc, 0, cw - maxwc, ch, colorGroup().base());
	pb->fillRect(0, rowPos(rowlast) + m_rowHeight, cw, ch, colorGroup().base());

	for(r = rowfirst; r <= rowlast; r++, ++it)
	{
		// get row position and height
		int rowp = rowPos(r);

		// Go through the columns in the row r
		// if we know from where to where, go through [colfirst, collast],
		// else go through all of them
		int colp, colw;
		colp = 0;
		int transly = rowp-cy;
		int c;

		if(m_bgAltering && r%2 != 0)
			pb->fillRect(0, transly, maxwc, m_rowHeight - 1, KGlobalSettings::alternateBackgroundColor());
		else
			pb->fillRect(0, transly, maxwc, m_rowHeight - 1, colorGroup().base());

		for(c = colfirst; c <= collast; c++)
		{
		// get position and width of column c
		colp = columnPos(c);
		colw = columnWidth(c);
		int translx = colp-cx;

		// Translate painter and draw the cell
		pb->saveWorldMatrix();
		pb->translate(translx, transly);
//	    	paintCell( pb, r, c, QRect(columnPos(c), rowPos(r), colw, m_rowHeight));
	    	paintCell( pb, it.current(), c, QRect(columnPos(c), rowPos(r), colw, m_rowHeight));
		  	pb->restoreWorldMatrix();
		}

	}
	delete pb;

	p->drawPixmap(cx,cy,*m_pBufferPm, 0,0,cw,ch);
}

void KexiTableView::paintCell(QPainter* p, KexiTableItem *item, int col, const QRect &cr, bool print)
{
	int w = cr.width();
	int h = cr.height();
	int x2 = w - 1;
	int y2 = h - 1;

//	p->setPen(colorGroup().button());

/*	if(m_bgAltering && !print && m_contents->findRef(item)%2 != 0)
	{
		QPen originalPen(p->pen());
		QBrush originalBrush(p->brush());

		p->setBrush(KGlobalSettings::alternateBackgroundColor());
		p->setPen(KGlobalSettings::alternateBackgroundColor());
		p->drawRect(0, 0, x2, y2);

		p->setPen(originalPen);
		p->setBrush(originalBrush);
	}
*/
	//	Draw our lines
	QPen pen(p->pen());
//	if(!print)
		p->setPen(QColor(200,200,200));
//	else
//		p->setPen(black);

	p->drawLine( x2, 0, x2, y2 );	// right
	p->drawLine( 0, y2, x2, y2 );	// bottom
	p->setPen(pen);

	//	If we are in the focus cell, draw indication
	if(m_pCurrentItem == item && col == m_curCol && !m_recordIndicator)
	{
		if (hasFocus() || viewport()->hasFocus())
			p->drawRect(0, 0, x2, y2);
	}

/*	if(m_pCurrentItem == item && m_recordIndicator)
	{
		p->setBrush(colorGroup().highlight());
		p->setPen(colorGroup().highlight());
		p->drawRect(0, 0, x2, y2);
	}
*/

	int x = 1;

//	int iOffset = 0;
	QPen fg(colorGroup().text());
	p->setPen(fg);
	if(item->isInsertItem())
	{
//		QFont f = p->font();
//		f.setBold(true);
//		p->setFont(f);
//		p->setPen(QPen(colorGroup().mid()));
		p->setPen(QColor(190,190,190));
//		iOffset = 3;
	}
	switch(columnType(col))
	{
		case QVariant::UInt:
		case QVariant::Int:
		{
			int num = item->getValue(col).toInt();
//			if(num < 0)
//				p->setPen(red);
//			p->drawText(x - (x+x) - 2, 2, w, h, AlignRight, QString::number(num));
//			qDebug("KexiTableView::paintCell(): mode: %i", m_pColumnModes->at(col));
			if(item->isInsertItem() && m_pColumnModes->at(col) == 3)  //yes that isn't beautiful
			{
				p->drawText(x, 1, w - (x+x) -4 - 2, h, AlignRight, "[Auto]");
			}
			else
			{
				p->drawText(x, 1, w - (x+x) -4 - 2, h, AlignRight, QString::number(num));
			}
//			p->drawRect(x - 1, 1, w - (x+x) - 1, h + 1);
			break;
		}
		case QVariant::Double:
		{
			QString f = KGlobal::locale()->formatNumber(item->getValue(col).toDouble());
			if(item->isInsertItem() && m_pColumnModes->at(col) == 3)  //yes that isn't beautiful
			{
				p->drawText(x, 1, w - (x+x) -4 - 2, h, AlignRight, "[Auto]");
			}
			else
			{
				p->drawText(x, 1, w - (x+x) -4 - 2, h, AlignRight, f);
			}
			break;
		}
		case QVariant::Bool:
		{
/*			QRect r(w/2 - style().pixelMetric(QStyle::PM_IndicatorWidth)/2 + x-1, 1, style().pixelMetric(QStyle::PM_IndicatorWidth), style().pixelMetric(QStyle::PM_IndicatorHeight));
			QPen pen(p->pen());		// bug in KDE HighColorStyle
			style().drawControl(QStyle::CE_CheckBox, p, this, r, colorGroup(), (item->getInt(col) ? QStyle::Style_On : QStyle::Style_Off) | QStyle::Style_Enabled);
			p->setPen(pen); */
			int s = QMAX(h - 5, 12);
			QRect r(w/2 - s/2 + x, h/2 - s/2, s, s);
			p->setPen(QPen(colorGroup().text(), 1));
			p->drawRect(r);
			if(item->getValue(col).asBool())
			{
				p->drawLine(r.x() + 2, r.y() + 2, r.right() - 1, r.bottom() - 1);
				p->drawLine(r.x() + 2, r.bottom() - 2, r.right() - 1, r.y() + 1);
			}

			break;
		}
		case QVariant::Date:
		{
			QString s = "";

			if(item->getValue(col).toDate().isValid())
			{
				#ifdef USE_KDE
				s = KGlobal::locale()->formatDate(item->getValue(col).toDate(), true);
				#else
				s = item->getDate(col).toString(Qt::LocalDate);
				#endif
				p->drawText(x, -1, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, s);
				break;
			}
			break;
		}
		case QVariant::StringList:
		{
			QStringList sl = m_pColumnDefaults->at(col)->toStringList();
			p->drawText(x, 0, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter,
				sl[item->getInt(col)]);
			break;
		}
		case QVariant::String:
		default:
		{
			p->drawText(x, -1, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, item->getText(col));
		}
	}
	p->setPen(fg);
	if(item->isInsertItem())
	{
		QFont f = p->font();
		f.setBold(false);
		p->setFont(f);
//		iOffset = 3;
	}
}

void KexiTableView::paintEmptyArea( QPainter *p, int cx, int cy, int cw, int ch )
{
	p->fillRect(cx, cy, cw, ch, colorGroup().brush(QColorGroup::Base));
}

void KexiTableView::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
	contentsMousePressEvent(e);

	if(m_pCurrentItem)
	{
		if(m_editOnDubleClick && columnEditable(m_curCol) && columnType(m_curCol) != QVariant::Bool)
		{
			createEditor(m_curRow, m_curCol, QString::null);
		}

		emit itemDblClicked(m_pCurrentItem, m_curCol);
	}
}

void KexiTableView::contentsMousePressEvent( QMouseEvent* e )
{
	if(m_numRows == 0)
		return;


	// get rid of editor
	if (m_pEditor)
		editorOk();

	// remember old focus cell
	int oldRow = m_curRow;
	int oldCol = m_curCol;

	// get new focus cell
	m_curRow = rowAt(e->pos().y());
	m_curCol = columnAt(e->pos().x());

	if(m_curRow == -1)
		m_curRow = m_numRows-1;

	if(m_curCol == -1)
		m_curCol = m_numCols-1;

	//	if we have a new focus cell, repaint
	if (( m_curRow != oldRow) || (m_curCol != oldCol ))
	{
		int cw = columnWidth( m_curCol );
		int rh = rowHeight( m_curRow );
		updateCell( m_curRow, m_curCol );
		ensureVisible( columnPos( m_curCol ) + cw / 2, rowPos( m_curRow ) + rh / 2, cw / 2, rh / 2 );
		updateCell( oldRow, oldCol );
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
//		kdDebug()<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) trying to get the current item"<<endl;
		emit itemSelected(m_pCurrentItem);
	}

//	kdDebug()<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) by now the current items should be set, if not -> error + crash"<<endl;
	if(e->button() == RightButton)
	{
    //show own context menu if configured
    if (updateContextMenu()) {
      selectRow(m_curRow);
    	m_pContextMenu->exec(e->globalPos());
    }
    else {
  		emit contextMenuRequested(m_pCurrentItem, m_curCol, e->globalPos());
//  		emit contextMenuRequested(m_pCurrentItem, m_curRow, e->globalPos());
    }
	}
	else if(e->button() == LeftButton)
	{
		if(columnType(m_curCol) == QVariant::Bool && columnEditable(m_curCol))
		{
			boolToggled();
			updateCell( m_curRow, m_curCol );
		}
		else if(columnType(m_curCol) == QVariant::StringList && columnEditable(m_curCol))
		{
			createEditor(m_curRow, m_curCol);
		}
	}
}

void KexiTableView::contentsMouseMoveEvent( QMouseEvent *e )
{
	// do the same as in mouse press
	int x,y;
	contentsToViewport(e->x(), e->y(), x, y);

	if(y > visibleHeight())
	{
		m_needAutoScroll = true;
		m_scrollTimer->start(70, false);
		m_scrollDirection = ScrollDown;
	}
	else if(y < 0)
	{
		m_needAutoScroll = true;
		m_scrollTimer->start(70, false);
		m_scrollDirection = ScrollUp;
	}
	else if(x > visibleWidth())
	{
		m_needAutoScroll = true;
		m_scrollTimer->start(70, false);
		m_scrollDirection = ScrollRight;
	}
	else if(x < 0)
	{
		m_needAutoScroll = true;
		m_scrollTimer->start(70, false);
		m_scrollDirection = ScrollLeft;
	}
	else
	{
		m_needAutoScroll = false;
		m_scrollTimer->stop();
		contentsMousePressEvent(e);
	}

}

void KexiTableView::contentsMouseReleaseEvent(QMouseEvent *e)
{
	if(m_needAutoScroll)
	{
		m_scrollTimer->stop();
	}
}

void KexiTableView::keyPressEvent(QKeyEvent* e)
{
	kdDebug() << "KexiTableView::KeyPressEvent()" << endl;

	if(m_pCurrentItem == 0 && m_numRows > 0)
	{
		m_curCol = m_curRow = 0;
		int cw = columnWidth(m_curCol);

		ensureVisible(columnPos(m_curCol) + cw / 2, rowPos(m_curRow) + m_rowHeight / 2, cw / 2, m_rowHeight / 2);
		updateCell(m_curRow, m_curCol);
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}
	else if(m_numRows == 0)
	{
		return;
	}

	// if a cell is just editing, do some special stuff
	if(m_pEditor)
	{
		if (e->key() == Key_Escape)
			editorCancel();
		else if (e->key() == Key_Return || e->key() == Key_Enter)
			editorOk();
		return;
	}

	if(e->key() == Key_Return || e->key() == Key_Enter)
	{
		emit itemReturnPressed(m_pCurrentItem, m_curCol);
	}

	int oldRow = m_curRow;
	int oldCol = m_curCol;

	// navigate in the header...
    switch (e->key())
    {
	case Key_Delete: 
		if (e->state()==Qt::ControlButton) {//remove current row
			removeRecord();
		}
		else {//remove all chars in the current cell
			if(columnType(m_curCol) != QVariant::Bool && columnEditable(m_curCol))
				createEditor(m_curRow, m_curCol, QString::null, false);
			if (m_pEditor && m_pEditor->isA("KexiInputTableEdit")) {
				static_cast<KexiInputTableEdit*>(m_pEditor)->clear();
			}
		}
		break;

	case Key_Shift:
	case Key_Alt:
	case Key_Control:
	case Key_Meta:
		e->ignore();
		break;

	case Key_Left:
		m_curCol = QMAX(0, m_curCol - 1);
		break;
	case Key_Right:
	case Key_Tab:
		m_curCol = QMIN(cols() - 1, m_curCol + 1);
		break;
    case Key_Up:
		m_curRow = QMAX(0, m_curRow - 1);
		break;
    case Key_Down:
		m_curRow = QMIN(rows() - 1, m_curRow + 1);
		break;
    case Key_Prior:
		m_curRow -= visibleHeight() / m_rowHeight;
		m_curRow = QMAX(0, m_curRow);
		break;
    case Key_Next:
		m_curRow += visibleHeight() / m_rowHeight;
		m_curRow = QMIN(rows()-1, m_curRow);
		break;
    case Key_Home:
		m_curRow = 0;
		break;
    case Key_End:
		m_curRow = m_numRows-1;
		break;

#ifndef _WIN32
	#warning this needs work!
#endif
	case Key_Enter:
	case Key_Return:
	case Key_F2:
		if(columnType(m_curCol) != QVariant::Bool && columnEditable(m_curCol))
			createEditor(m_curRow, m_curCol, QString::null, false);
		break;

	case Key_Backspace:
		if(columnType(m_curCol) != QVariant::Bool && columnEditable(m_curCol))
			createEditor(m_curRow, m_curCol, QString::null, true);
		break;
	case Key_Space:
		if(columnType(m_curCol) == QVariant::Bool && columnEditable(m_curCol))
		{
			boolToggled();
			break;
		}
    default:
   	 	qDebug("KexiTableView::KeyPressEvent(): default");
		if (e->text().isEmpty() || !e->text().isEmpty() && !e->text()[0].isPrint() ) {
			kdDebug() << "NOT PRINTABLE" <<endl;
			e->ignore();
			return;
		}
		if(columnType(m_curCol) != QVariant::Bool && columnEditable(m_curCol))
		{
			qDebug("KexiTableView::KeyPressEvent(): ev pressed");
//			if (e->text()[0].isPrint())
			createEditor(m_curRow, m_curCol, e->text(), false);
		}
	}
	// if focus cell changes, repaint
	if ( m_curRow != oldRow || m_curCol != oldCol )
	{
		int cw = columnWidth(m_curCol);

		ensureVisible(columnPos(m_curCol) + cw / 2, rowPos(m_curRow) + m_rowHeight / 2, cw / 2, m_rowHeight / 2);
		updateCell(oldRow, oldCol);
		updateCell(m_curRow, m_curCol);
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}
}

void KexiTableView::emitSelected()
{
	if(m_pCurrentItem)
		emit itemSelected(m_pCurrentItem);
}

void KexiTableView::boolToggled()
{
	int s = m_pCurrentItem->getInt(m_curCol);
	QVariant oldValue=m_pCurrentItem->getValue(m_curCol);
	m_pCurrentItem->setInt(m_curCol, (s ? 0 : 1));
	updateCell(m_curRow, m_curCol);
	emit itemChanged(m_pCurrentItem, m_curCol,oldValue);
	emit itemChanged(m_pCurrentItem, m_curCol);
}

void KexiTableView::selectNext()
{
//	int oldRow = m_curRow;
//	m_curRow = QMIN( rows() - 1, m_curRow + 1 );
	selectRow( QMIN( rows() - 1, m_curRow + 1 ) );
/*js
	if(m_curRow != oldRow)
	{
		int rh = rowHeight(m_curRow);

		ensureVisible(0, rowPos(m_curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, m_curCol);
			updateCell(m_curRow, m_curCol);
		}
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}*/
}

void KexiTableView::selectRow(int row)
{
	int oldRow = m_curRow;

	if(m_curRow != row)
	{
		m_curRow = row;
		int rh = rowHeight(m_curRow);

		ensureVisible(0, rowPos(m_curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, m_curCol);
			updateCell(m_curRow, m_curCol);
		}
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}
}

void KexiTableView::selectPrev()
{
//	int oldRow = m_curRow;

//	m_curRow = QMAX( 0, m_curRow - 1 );
	selectRow( QMAX( 0, m_curRow - 1 ) );
	/*js
	if(m_curRow != oldRow)
	{
		int rh = rowHeight(m_curRow);

		ensureVisible(0, rowPos(m_curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, m_curCol);
			updateCell(m_curRow, m_curCol);
		}
		m_pVerticalHeader->setCurrentRow(m_curRow);
		m_pCurrentItem = itemAt(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}*/
}

void KexiTableView::gotoNext()
{
	int oldCol = m_curCol;
	m_curCol = QMIN( cols() - 1, m_curCol + 1 );

	if(m_curCol != oldCol)
	{
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(m_curRow, oldCol);
			updateCell(m_curRow, m_curCol);
		}
	}
	else
	{
		selectNext();
	}
}

void KexiTableView::createEditor(int row, int col, QString addText/* = QString::null*/, bool backspace/* = false*/)
{
//	QString val;
	QVariant val;
/*	switch(columnType(col))
	{
		case QVariant::Date:
			#ifdef USE_KDE
//			val = KGlobal::locale()->formatDate(m_pCurrentItem->getDate(col), true);

			#else
//			val = m_pCurrentItem->getDate(col).toString(Qt::LocalDate);
			#endif
			break;

		default:
//			val = m_pCurrentItem->getText(m_curCol);
			val = m_pCurrentItem->getValue(m_curCol);

			break;
	}*/

	val = m_pCurrentItem->getValue(m_curCol);

	//it's getting ugly :)

	switch(columnType(col))
	{
		case QVariant::ByteArray:
			m_pEditor = new KexiBlobTableEdit(val.toByteArray(), viewport(), "inPlaceEd");
			m_pEditor->resize(columnWidth(m_curCol)-1, 150);
			moveChild(m_pEditor, columnPos(m_curCol), rowPos(m_curRow));
			m_pEditor->show();
			m_pEditor->setFocus();
			return;
		case QVariant::StringList:
			m_pEditor = new KexiComboBoxTableEdit(static_cast<KexiDBField::ColumnType>(val.toInt()),
				m_pColumnDefaults->at(col)->toStringList(), viewport(), "inPlaceEd");
			break;
		case QVariant::Date:
			m_pEditor = new KexiDateTableEdit(val, viewport(), "inPlaceEd");
			kdDebug() << "date editor created..." << endl;
			break;
		default:
			m_pEditor = new KexiInputTableEdit(val, columnType(col), addText, false, viewport(), "inPlaceEd",
			 m_pColumnDefaults->at(col)->toStringList());
			static_cast<KexiInputTableEdit*>(m_pEditor)->end(false);
			if(backspace)
				static_cast<KexiInputTableEdit*>(m_pEditor)->backspace();

			break;
	}

	moveChild(m_pEditor, columnPos(m_curCol), rowPos(m_curRow)-1);
	m_pEditor->resize(columnWidth(m_curCol)-1, rowHeight(m_curRow));
//	moveChild(m_pEditor, columnPos(m_curCol), rowPos(m_curRow));
	m_pEditor->show();
	m_pEditor->setFocus();
}

void KexiTableView::focusInEvent(QFocusEvent*)
{
	updateCell(m_curRow, m_curCol);
}


void KexiTableView::focusOutEvent(QFocusEvent*)
{
	updateCell(m_curRow, m_curCol);
}

bool KexiTableView::focusNextPrevChild(bool next)
{
	if (m_pEditor)
		return true;
	return QScrollView::focusNextPrevChild(next);
}

void KexiTableView::resizeEvent(QResizeEvent *e)
{
	QScrollView::resizeEvent(e);
	updateGeometries();
}

void KexiTableView::showEvent(QShowEvent *e)
{
	QScrollView::showEvent(e);
	QRect r(cellGeometry(rows() - 1, cols() - 1 ));
	resizeContents(r.right() + 1, r.bottom() + 1);
	updateGeometries();
}


void KexiTableView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	for(QStringList::Iterator it = m_dropFilters.begin(); it != m_dropFilters.end(); it++)
	{
		if(e->provides((*it).latin1()))
		{
			e->acceptAction(true);
			return;
		}
	}
	e->acceptAction(false);
}

void KexiTableView::contentsDropEvent(QDropEvent *ev)
{
	emit dropped(ev);
}

void KexiTableView::updateCell(int row, int col)
{

	if(!m_recordIndicator)
	{
		updateContents(cellGeometry(row, col));
	}
	else
	{
		for(int i = 0; i < m_numCols; i++)
		{
			updateContents(cellGeometry(row, i));
		}
	}

}

void KexiTableView::columnSort(int col)
{
	bool i = false;
	QVariant hint;
	if(m_pInsertItem)
	{
		i = true;
		hint = m_pInsertItem->getHint();
//		delete m_pInsertItem;
		remove(m_pInsertItem);
		m_pInsertItem = 0;
		m_pVerticalHeader->removeLabel(rows());
	}

	if(m_sortedColumn == col)
		m_sortOrder = !m_sortOrder;
	else
		m_sortOrder = true;
	m_sortedColumn = col;
	m_pTopHeader->setSortIndicator(col, m_sortOrder);
	m_contents->setSorting(col, m_sortOrder, columnType(col));
	sort();

	if(i)
	{
		KexiTableItem *insert = new KexiTableItem(this);
		insert->setHint(hint);
		insert->setInsertItem(true);
		m_pInsertItem = insert;

	}
//	updateContents( 0, 0, viewport()->width(), viewport()->height());
}

void KexiTableView::columnWidthChanged( int col, int, int )
{
	QSize s(tableSize());
	int w = contentsWidth();
	viewport()->setUpdatesEnabled(false);
	resizeContents( s.width(), s.height() );
	viewport()->setUpdatesEnabled(true);
	if (contentsWidth() < w)
		updateContents(contentsX(), 0, viewport()->width(), contentsHeight());
//		repaintContents( s.width(), 0, w - s.width() + 1, contentsHeight(), TRUE );
	else
	//	updateContents( columnPos(col), 0, contentsWidth(), contentsHeight() );
		updateContents(columnPos(col), 0, viewport()->width(), contentsHeight());
	//	viewport()->repaint();
	if (m_pEditor)
	{
		m_pEditor->resize(columnWidth(m_curCol)-1, rowHeight(m_curRow)-1);
		moveChild(m_pEditor, columnPos(m_curCol), rowPos(m_curRow));
	}
	updateGeometries();
}

void KexiTableView::updateGeometries()
{
	QSize ts = tableSize();
	if (m_pTopHeader->offset() && ts.width() < m_pTopHeader->offset() + m_pTopHeader->width())
		horizontalScrollBar()->setValue(ts.width() - m_pTopHeader->width());

//	m_pVerticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
	m_pTopHeader->setGeometry(leftMargin() + 1, 1, visibleWidth(), topMargin());
	m_pVerticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
}

int KexiTableView::columnWidth(int col) const
{
    return m_pTopHeader->sectionSize(col);
}

int KexiTableView::rowHeight(int row) const
{
	return m_rowHeight;
}

int KexiTableView::columnPos(int col) const
{
    return m_pTopHeader->sectionPos(col);
}

int KexiTableView::rowPos(int row) const
{
	return m_rowHeight*row;
}

int KexiTableView::columnAt(int pos) const
{
    return m_pTopHeader->sectionAt(pos);
}

int KexiTableView::rowAt(int pos) const
{
	pos /=m_rowHeight;
	if (pos < 0)
		return 0;
	if (pos >= m_numRows)
		return -1;
	return pos;
}

QRect KexiTableView::cellGeometry(int row, int col) const
{
    return QRect(columnPos(col), rowPos(row),
		  columnWidth(col), rowHeight(row));
}

QSize KexiTableView::tableSize() const
{
	if (rows() > 0 && cols() > 0)
	    return QSize( columnPos( cols() - 1 ) + columnWidth( cols() - 1 ),
			rowPos( rows()-1 ) + rowHeight( rows() - 1 ));
	return QSize(0,0);
}

int KexiTableView::rows() const
{
	return m_numRows;
}

int KexiTableView::cols() const
{
    return m_pTopHeader->count();
}

void KexiTableView::setCursor(int row, int col/*=-1*/)
{
	// get rid of editor
	if (m_pEditor)
		editorOk();
	if(rows() <= 0)
	{
		m_curRow=0;
		m_pCurrentItem=0;
		m_pVerticalHeader->setCurrentRow(-1);
		return;
	}

	int oldRow = m_curRow;
	int oldCol = m_curCol;

	if(col>=0)
	{
		m_curCol = QMAX(0, col);
		m_curCol = QMIN(cols() - 1, m_curCol);
	}
	m_curRow = QMAX( 0, row);
	m_curRow = QMIN( rows() - 1, m_curRow);

	m_pCurrentItem = itemAt(m_curRow);
	if ( m_curRow != oldRow || m_curCol != oldCol )
	{
//		int cw = columnWidth( m_curCol );
		int rh = rowHeight( m_curRow );
//		ensureVisible( columnPos( m_curCol ) + cw / 2, rowPos( m_curRow ) + rh / 2, cw / 2, rh / 2 );
//		center(columnPos(m_curCol) + cw / 2, rowPos(m_curRow) + rh / 2, cw / 2, rh / 2);
		ensureVisible(columnPos(m_curCol), rowPos(m_curRow), columnWidth(m_curCol), rh);
		updateCell( oldRow, oldCol );
		updateCell( m_curRow, m_curCol );
		m_pVerticalHeader->setCurrentRow(m_curRow);
		emit itemSelected(m_pCurrentItem);
	}
}

void KexiTableView::editorOk()
{
	if (!m_pEditor)
		return;
//	m_pCurrentItem->setText(m_curCol, m_pEditor->text());
	QVariant oldValue=m_pCurrentItem->getValue(m_curCol);
	m_pCurrentItem->setValue(m_curCol, m_pEditor->value());
	editorCancel();
	emit itemChanged(m_pCurrentItem, m_curCol,oldValue);
	emit itemChanged(m_pCurrentItem, m_curCol);
}

void KexiTableView::editorCancel()
{
	if (!m_pEditor)
		return;

	delete m_pEditor;
	m_pEditor = 0;
	viewport()->setFocus();
}

void KexiTableView::setAdditionPolicy(AdditionPolicy policy)
{
	m_additionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::AdditionPolicy KexiTableView::additionPolicy()
{
	return m_additionPolicy;
}

void KexiTableView::setDeletionPolicy(DeletionPolicy policy)
{
	m_deletionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::DeletionPolicy KexiTableView::deletionPolicy()
{
	return m_deletionPolicy;
}

/*! Updates visibility/accesibility of popup menu items,
  returns false if no items are visible after update. */
bool KexiTableView::updateContextMenu()
{
  // delete m_pContextMenu;
  //  m_pContextMenu = 0L;
//  m_pContextMenu->clear();
//	if(m_pCurrentItem && m_pCurrentItem->isInsertItem())
//    return;

//	if(m_additionPolicy != NoAdd || m_deletionPolicy != NoDelete)
//	{
//		m_pContextMenu = new QPopupMenu(this);
  m_pContextMenu->setItemVisible(menu_id_addRecord, m_additionPolicy != NoAdd);
  m_pContextMenu->setItemVisible(menu_id_removeRecord, m_deletionPolicy != NoDelete
    && m_pCurrentItem && !m_pCurrentItem->isInsertItem());
  for (int i=0; i<(int)m_pContextMenu->count(); i++) {
    if (m_pContextMenu->isItemVisible( m_pContextMenu->idAt(i) ))
      return true;
  }
  return false;
}

void KexiTableView::slotAutoScroll()
{
	if(m_needAutoScroll)
	{
		switch(m_scrollDirection)
		{
			case ScrollDown:
				setCursor(m_curRow + 1, m_curCol);
				break;

			case ScrollUp:
				setCursor(m_curRow - 1, m_curCol);
				break;
			case ScrollLeft:
				setCursor(m_curRow, m_curCol - 1);
				break;

			case ScrollRight:
				setCursor(m_curRow, m_curCol + 1);
				break;
		}
	}
}

void KexiTableView::inserted()
{
	if(itemAt(m_numRows)->isInsertItem())
		m_pVerticalHeader->addLabel("*", m_rowHeight);
	else
		m_pVerticalHeader->addLabel("", m_rowHeight);

}

#ifndef KEXI_NO_PRINT
void
KexiTableView::print(KPrinter &printer)
{
//	printer.setFullPage(true);

	int leftMargin = printer.margins().width() + 2 + m_rowHeight;
	int topMargin = printer.margins().height() + 2;
//	int bottomMargin = topMargin + ( printer.realPageSize()->height() * printer.resolution() + 36 ) / 72;
	int bottomMargin = 0;
	kdDebug() << "KexiTableView::print: bottom = " << bottomMargin << endl;

	QPainter p(&printer);

	KexiTableItem *i;
	int width = leftMargin;
	for(int col=0; col < cols(); col++)
	{
		p.fillRect(width, topMargin - m_rowHeight, columnWidth(col), m_rowHeight, QBrush(gray));
		p.drawRect(width, topMargin - m_rowHeight, columnWidth(col), m_rowHeight);
		p.drawText(width, topMargin - m_rowHeight, columnWidth(col), m_rowHeight, AlignLeft | AlignVCenter, m_pTopHeader->label(col));
		width = width + columnWidth(col);
	}

	int yOffset = topMargin;
	int row = 0;
	int right = 0;
	for(i = m_contents->first(); i; i = m_contents->next())
	{
		if(!i->isInsertItem())
		{	kdDebug() << "KexiTableView::print: row = " << row << " y = " << yOffset << endl;
			int xOffset = leftMargin;
			for(int col=0; col < cols(); col++)
			{
				kdDebug() << "KexiTableView::print: col = " << col << " x = " << xOffset << endl;
				p.saveWorldMatrix();
				p.translate(xOffset, yOffset);
				paintCell(&p, i, col, QRect(0, 0, columnWidth(col) + 1, m_rowHeight), true);
				p.restoreWorldMatrix();
//			p.drawRect(xOffset, yOffset, columnWidth(col), m_rowHeight);
				xOffset = xOffset + columnWidth(col);
				right = xOffset;
			}

			row++;
			yOffset = topMargin  + row * m_rowHeight;
		}

		if(yOffset > 900)
		{
			p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
			p.drawLine(leftMargin, topMargin, right - 1, topMargin);
			printer.newPage();
			yOffset = topMargin;
			row = 0;
		}
	}
	p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
	p.drawLine(leftMargin, topMargin, right - 1, topMargin);

//	p.drawLine(60,60,120,150);
	p.end();
}
#endif //!KEXI_NO_PRINT

void
KexiTableView::takeInsertItem()
{
//	delete m_pInsertItem;
	m_pInsertItem = 0;
}


KexiTableHeader* KexiTableView::recordMarker()
{
	return m_pVerticalHeader;
}

QString KexiTableView::column(int section) { return m_pTopHeader->label(section);}


#include "kexitableview.moc"
