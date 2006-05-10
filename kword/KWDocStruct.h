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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef docstruct_h
#define docstruct_h

#include <k3listview.h>
#include <QWidget>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QPaintEvent>

class KWDocument;
class KWFormulaFrameSet;
class KWPictureFrameSet;
class KWPartFrameSet;
class KWGUI;
class QWidget;
class Q3VBoxLayout;
class KWFrame;
class KWFrameSet;
class KWTextParag;

class KWDocListViewItem : public K3ListViewItem
{
public:
    enum ItemType {
        IT_BASE = FT_BASE,
        IT_TEXT = FT_TEXT,
        IT_PICTURE = FT_PICTURE,
        IT_PART = FT_PART,
        IT_FORMULA = FT_FORMULA,
        IT_CLIPART = FT_CLIPART,
        IT_TABLE = FT_TABLE,
        IT_ROOT = 11,
        IT_TEXTFRAME = 12,
        IT_PARAG = 13
    };

    KWDocListViewItem(Q3ListViewItem *_parent, const QString &_text);
    KWDocListViewItem(Q3ListViewItem *_parent, Q3ListViewItem *_after, const QString &_text);
    KWDocListViewItem(Q3ListView *_parent, const QString &_text);
    virtual ItemType itemType() { return IT_BASE; }
    virtual void selectItem() {}
    virtual void editItem() {}
    virtual void deleteItem() {}
    virtual void editProperties() {}
    virtual void contextMenu(Q3ListViewItem *, const QPoint &, int) {}

protected:
    KWDocument* doc();
    KWGUI* gui();
    void deleteAllChildren();
};

/******************************************************************/
/* Class: KWDocStructParagItem                                    */
/******************************************************************/

class KWDocStructParagItem : public KWDocListViewItem
{
public:
    KWDocStructParagItem(Q3ListViewItem* parent, const QString& text, KWTextParag* parag);
    KWDocStructParagItem(Q3ListViewItem* parent, Q3ListViewItem* after, const QString& text, KWTextParag* parag);
    virtual ItemType itemType() { return IT_PARAG; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

    KWTextParag* parag() const { return m_parag; }

protected:
    KWTextParag* m_parag;
};

/******************************************************************/
/* Class: KWDocStructTextFrameItem                                */
/******************************************************************/

class KWDocStructTextFrameItem : public KWDocListViewItem
{
public:
    KWDocStructTextFrameItem(Q3ListViewItem* parent, const QString& text,
        KWTextFrameSet* frameset, KWFrame* frame );
    KWDocStructTextFrameItem(Q3ListViewItem* parent, Q3ListViewItem* after, const QString& text,
        KWTextFrameSet* frameset, KWFrame* frame );
    virtual int ItemType() { return IT_TEXTFRAME; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

    KWFrame* frame() const { return m_frame; }
    KWTextFrameSet* frameSet() const { return m_frameset; };
    void setupTextParags();
    KWDocStructParagItem* findTextParagItem(const KWTextParag* parag);

protected:
    KWFrame* m_frame;
    KWTextFrameSet* m_frameset;
};


/******************************************************************/
/* Class: KWDocStructTextFrameSetItem                                 */
/******************************************************************/

class KWDocStructTextFrameSetItem : public KWDocListViewItem
{
public:
    KWDocStructTextFrameSetItem(Q3ListViewItem* parent, const QString& text, KWTextFrameSet* frameset);
    KWDocStructTextFrameSetItem(Q3ListViewItem* parent, Q3ListViewItem* after, const QString& text,
        KWTextFrameSet* frameset );
    virtual int ItemType() { return IT_TEXT; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

    KWTextFrameSet* frameSet() const { return m_frameset; };
    void setupTextFrames();
    KWDocStructTextFrameItem* findTextFrameItem(const KWFrame* frame);

protected:
    KWTextFrameSet* m_frameset;

};

/******************************************************************/
/* Class: KWDocStructTableItem                                    */
/******************************************************************/

class KWDocStructTableItem : public KWDocListViewItem
{
public:
    KWDocStructTableItem(Q3ListViewItem* parent, const QString& text, KWTableFrameSet* table );
    KWDocStructTableItem(Q3ListViewItem* parent, Q3ListViewItem* after, const QString& text,
        KWTableFrameSet* table );
    virtual int ItemType() { return IT_TABLE; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

    KWTableFrameSet* table() const { return m_table; };
    void setupCells();
    KWDocStructTextFrameItem* findCellItem(const KWTextFrameSet* cell);

protected:
    KWTableFrameSet* m_table;
};

/******************************************************************/
/* Class: KWDocStructFormulaItem                                  */
/******************************************************************/

class KWDocStructFormulaItem : public KWDocListViewItem
{
public:
    KWDocStructFormulaItem(Q3ListViewItem* parent, const QString& text, KWFormulaFrameSet* form);
    virtual int ItemType() { return IT_FORMULA; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

protected:
    KWFormulaFrameSet* m_form;
};


/******************************************************************/
/* Class: KWDocStructPictureItem                                  */
/******************************************************************/

class KWDocStructPictureItem : public KWDocListViewItem
{
public:
    KWDocStructPictureItem(Q3ListViewItem* parent, const QString& text, KWPictureFrameSet* pic);
    virtual int ItemType() { return IT_PICTURE; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

protected:
    KWPictureFrameSet* m_pic;
};

/******************************************************************/
/* Class: KWDocStructPartItem                                     */
/******************************************************************/

class KWDocStructPartItem : public KWDocListViewItem
{
public:
    KWDocStructPartItem(Q3ListViewItem* parent, const QString& text, KWPartFrameSet* part);
    virtual int ItemType() { return IT_PART; }
    virtual void selectItem();
    virtual void editItem();
    virtual void deleteItem();
    virtual void editProperties();
    virtual void contextMenu(Q3ListViewItem* item, const QPoint& p, int);

protected:
    KWPartFrameSet* m_part;
};

/******************************************************************/
/* Class: KWDocStructRootItem                                     */
/******************************************************************/

class KWDocStructRootItem : public KWDocListViewItem
{
public:
    KWDocStructRootItem(Q3ListView* parent, const QString& text, TypeStructDocItem type);

    virtual int ItemType() { return IT_ROOT; }
    void setupArrangement();
    void setupTextFrameSets();
    void setupTables();
    void setupPictures();
    void setupEmbedded();
    void setupFormulaFrames();
    virtual void setOpen(bool o);

protected:
    KWDocStructTextFrameSetItem* findTextFrameSetItem(const KWFrameSet* frameset);
    KWDocStructTableItem* findTableItem(const KWFrameSet* frameset);

    TypeStructDocItem m_type;

};

/******************************************************************/
/* Class: KWDocStructTree                                         */
/******************************************************************/

class KWDocStructTree : public K3ListView
{
    Q_OBJECT

public:
    KWDocStructTree( QWidget* parent, KWDocument* doc, KWGUI* gui );
    ~KWDocStructTree();

    void setup();
    void refreshTree(int _type);
    QSize minimumSizeHint() const {
        return QSize( 0, 0 );
    }
    void selectItem();
    void editItem();
    void deleteItem();
    void editProperties();

    KWDocument* doc() { return m_doc; };
    KWGUI* gui() { return m_gui; };

public slots:
    void slotContextMenu(K3ListView* l, Q3ListViewItem* i, const QPoint& p);
    void slotRightButtonClicked(Q3ListViewItem*, const QPoint&, int);
    void slotDoubleClicked(Q3ListViewItem* item);
    void slotReturnPressed(Q3ListViewItem* item);

protected:
    KWDocument* m_doc;
    KWGUI* m_gui;

    KWDocStructRootItem *arrangement, *tables, *pictures, *textfrms, *embedded, *formulafrms;
};

/******************************************************************/
/* Class: KWDocStruct                                             */
/******************************************************************/

class KWDocStruct : public QWidget
{
    Q_OBJECT

public:
    KWDocStruct( QWidget* parent, KWDocument* doc, KWGUI* gui );
    void selectItem();
    void editItem();
    void deleteItem();
    void editProperties();
    /** Refresh the specified branches of the tree.  Actual refreshing is deferred
        until the paint event. */
    void refreshTree(int _type);
    /** Set focus to the tree, if visible. */
    void setFocusHere();

public slots:
    /** Refresh the entire tree. */
    void refreshEntireTree();

protected:
    KWDocStructTree* m_tree;
    Q3VBoxLayout* m_layout;

    virtual void paintEvent(QPaintEvent* ev);

    KWDocument* m_doc;
    KWGUI* m_gui;
    int dirtyTreeTypes;
};

#endif
