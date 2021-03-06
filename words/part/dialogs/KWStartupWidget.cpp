/* This file is part of the Calligra project
 * Copyright (C) 2005, 2007-2009 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KWStartupWidget.h"

#include "KWDocumentColumns.h"
#include "../KWPage.h"
#include "../KWDocument.h"

#include <KoPageLayoutWidget.h>
#include <KoPagePreviewWidget.h>

KWStartupWidget::KWStartupWidget(QWidget *parent, KWDocument *doc, const KoColumns &columns)
        : QWidget(parent),
        m_unit(doc->unit())
{
    widget.setupUi(this);
    m_columns = columns;
    m_layout.leftMargin = MM_TO_POINT(30);
    m_layout.rightMargin = MM_TO_POINT(30);
    m_layout.topMargin = MM_TO_POINT(25);
    m_layout.bottomMargin = MM_TO_POINT(25);
    m_doc = doc;

    setFocusProxy(widget.createButton);

    QVBoxLayout *lay = new QVBoxLayout(widget.sizeTab);
    m_sizeWidget = new KoPageLayoutWidget(widget.sizeTab, m_layout);
    m_sizeWidget->showPageSpread(true);
    m_sizeWidget->setUnit(m_unit);
    lay->addWidget(m_sizeWidget);

    lay = new QVBoxLayout(widget.columnsTab);
    m_columnsWidget = new KWDocumentColumns(widget.columnsTab, m_columns);
    m_columnsWidget->setUnit(m_unit);
    m_columnsWidget->setShowPreview(false);
    lay->addWidget(m_columnsWidget);

    lay = new QVBoxLayout(widget.previewPane);
    widget.previewPane->setLayout(lay);
    KoPagePreviewWidget *prev = new KoPagePreviewWidget(widget.previewPane);
    lay->addWidget(prev);
    prev->setColumns(columns);
    prev->setPageLayout(m_layout);

    connect(m_sizeWidget, SIGNAL(layoutChanged(KoPageLayout)), this, SLOT(sizeUpdated(KoPageLayout)));
    connect(widget.createButton, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    connect(m_sizeWidget, SIGNAL(unitChanged(KoUnit)), this, SLOT(unitChanged(KoUnit)));
    connect(m_columnsWidget, SIGNAL(columnsChanged(KoColumns)), prev, SLOT(setColumns(KoColumns)));
    connect(m_columnsWidget, SIGNAL(columnsChanged(KoColumns)), this, SLOT(columnsUpdated(KoColumns)));
    connect(m_sizeWidget, SIGNAL(layoutChanged(KoPageLayout)), prev, SLOT(setPageLayout(KoPageLayout)));
}

void KWStartupWidget::unitChanged(const KoUnit &unit)
{
    m_unit = unit;
    m_columnsWidget->setUnit(unit);
}

void KWStartupWidget::sizeUpdated(const KoPageLayout &layout)
{
    m_layout = layout;
}

void KWStartupWidget::columnsUpdated(const KoColumns &columns)
{
    m_columns = columns;
}

void KWStartupWidget::buttonClicked()
{
    m_doc->initEmpty();

    KWPageStyle style = m_doc->pageManager()->defaultPageStyle();
    Q_ASSERT(style.isValid());
    style.setColumns(m_columns);
    style.setPageLayout(m_layout);
    m_doc->setUnit(m_unit);

    m_doc->relayout();

    emit documentSelected();
}
