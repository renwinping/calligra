/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef presstructview_h
#define presstructview_h

#include <qdialog.h>
#include <qlistview.h>
#include <qlabel.h>

class KPresenterDoc;
class KPresenterView;
class QSplitter;
class QCheckBox;
class QResizeEvent;
class KPBackGround;
class KPObject;
class KPPresStructObjectItem;

/******************************************************************
 *
 * Class: KPSlidePreview
 *
 ******************************************************************/

class KPSlidePreview : public QLabel
{
    Q_OBJECT

public:
    KPSlidePreview( QWidget *parent, KPresenterDoc *_doc, KPresenterView *_view );

public slots:
    void setPage( QListViewItem *item );

protected:
    KPresenterDoc *doc;
    KPresenterView *view;

};

/******************************************************************
 *
 * Class: KPPresStructObjectItem
 *
 ******************************************************************/

class KPPresStructObjectItem : public QListViewItem
{
public:
    KPPresStructObjectItem( QListView *parent );
    KPPresStructObjectItem( QListViewItem *parent );

    void setPage( KPBackGround *p, int pgnum );
    void setObject( KPObject *o, int num );
    KPBackGround *getPage();
    int getPageNum();
    KPObject *getObject();

protected:
    KPBackGround *page;
    KPObject *object;
    int pageNum;

};

/******************************************************************
 *
 * Class: KPPresStructView
 *
 ******************************************************************/

class KPPresStructView : public QDialog
{
    Q_OBJECT

public:
    KPPresStructView( QWidget *parent, const char *name,
                      KPresenterDoc *_doc, KPresenterView *_view );

protected:
    void setupSlideList();
    void setupPagePreview();
    void resizeEvent( QResizeEvent *e );

    KPresenterDoc *doc;
    KPresenterView *view;
    QListView *slides;
    QSplitter *hsplit;
    QCheckBox *showPreview;
    KPSlidePreview *slidePreview;

protected slots:
    void makeStuffVisible( QListViewItem *item );
    
signals:
    void presStructViewClosed();

};

#endif
