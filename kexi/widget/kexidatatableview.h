/* This file is part of the KDE project
   Copyright (C) 2003   Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003   Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003   Jaroslaw Staniek <js@iidea.pl>

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
 */

#ifndef KEXIDATATABLEVIEW_H
#define KEXIDATATABLEVIEW_H

#include <tableview/kexitableview.h>

class KexiTableItem;
class QVariant;
class KXMLGUIClient;

namespace KexiDB {
	class Cursor;
}

/**
 * database aware table widget
 */
class KEXIEXTWIDGETS_EXPORT KexiDataTableView : public KexiTableView
{
	Q_OBJECT

	public:
		/**
		 * creates a blank widget
		 */
		KexiDataTableView(QWidget *parent, const char *name =0);
		/*! Creates a table widget and fills it using data from \a cursor.
		 Cursor will be opened (with open()) if it is not yet opened.
		 Cursor must be defined on query schema, not raw statement (see Connection::prepareQuery()
		 and Connection::executeQuery()), otherwise the table view remain not filled with data.
		 */
		KexiDataTableView(QWidget *parent, const char *name, KexiDB::Cursor *cursor);

		~KexiDataTableView();

		virtual void initActions(KActionCollection *col);

		/*! Fills table view with data using \a cursor. \return true on success.
		*/
		bool setData(KexiDB::Cursor *cursor);

		/*! \return cursor used as data source for this table view, 
		 or NULL if no valid cursor is defined. */
		KexiDB::Cursor *cursor() { return m_cursor; }

		/**
		 * @returns the number of records in the data set, (if data set is present)
		 * @note not all of the records have to be processed
		 */
		int recordCount() { return m_data->count(); }
//js		int		records() { return m_records; }

		bool readOnly();

		#ifndef KEXI_NO_PRINT
//		virtual void print(KPrinter &printer);
		#endif

	protected:
		void		init();
//		virtual QSize	tableSize() const;
//js		void		appendInsertItem();

		/*! Reimplemented: called by deleteItem() - we are deleting data associated with \a item. */
//		virtual bool beforeDeleteItem(KexiTableItem *item);

	protected slots:
//		void		recordInsertFinished(KexiDBUpdateRecord*);
//js		void		slotMoving(int);
//js		void		insertNext();

//js		void		slotItemChanged(KexiTableItem*, int, QVariant);

	private:
		//db stuff
		KexiDB::Cursor	*m_cursor;

//		KXMLGUIClient *m_guiClient;
//		bool		m_first;
//use m_record->database() instead:		KexiDB		*m_db;
//js		int		m_maxRecord;
//js		int		m_records;

//		QMap<KexiDBUpdateRecord*,KexiTableItem*> m_insertMapping;
};

#endif
