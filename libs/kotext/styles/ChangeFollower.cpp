/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; version 2.
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

#include "ChangeFollower.h"
#include "KoCharacterStyle.h"
#include "KoParagraphStyle.h"

#include <QVector>
#include <QTextDocument>

ChangeFollower::ChangeFollower(QTextDocument *parent, KoStyleManager *manager)
    : QObject(parent),
    m_document(parent),
    m_styleManager(manager)
{
}

ChangeFollower::~ChangeFollower() {
    if(m_styleManager)
        m_styleManager->remove(this);
}

void ChangeFollower::processUpdates(const QList<int> &changedStyles) {
    if(m_styleManager == 0) {
        // since the stylemanager would be the one calling this method, I doubt this
        // will ever happen.  But better safe than sorry..
        deleteLater();
        return;
    }

    foreach(QTextFormat format, m_document->allFormats()) {
        int id = format.intProperty(KoParagraphStyle::StyleId);
        if(id <= 0)
            continue;
        if(changedStyles.contains(id)) {
            if(format.isBlockFormat()) { // aka parag
                KoParagraphStyle *style = m_styleManager->paragraphStyle(id);
                Q_ASSERT(style);
                QTextBlockFormat bf = format.toBlockFormat();
                style->applyStyle(bf);
            }
            else if(format.isCharFormat()) {
                KoCharacterStyle *style = m_styleManager->characterStyle(id);
                Q_ASSERT(style);
                QTextCharFormat cf = format.toCharFormat();
                style->applyStyle(cf);
            }
        }
    }
}
