/* This file is part of the KDE project
*
* Copyright (C) 2011 Paul Mendez <paulestebanms@gmail.com>
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

#include "KPrEditCustomSlideShowsCommand.h"
#include "KPrDocument.h"
#include "KoPAPageBase.h"
#include "KPrCustomSlideShowsModel.h"

KPrEditCustomSlideShowsCommand::KPrEditCustomSlideShowsCommand(
    KPrDocument * doc, KPrCustomSlideShowsModel *model, QString name, QList<KoPAPageBase *> newCustomShow, QUndoCommand *parent)
: QUndoCommand( parent )
, m_doc(doc)
, m_model(model)
, m_name(name)
, m_newCustomShow(newCustomShow)
, m_oldCustomShow(doc->customSlideShows()->getByName(name))
{
    setText( i18n("Edit custom slide shows") );
}

KPrEditCustomSlideShowsCommand::~KPrEditCustomSlideShowsCommand()
{

}

void KPrEditCustomSlideShowsCommand::redo()
{
    m_model->updateCustomShow(m_name, m_newCustomShow);
    m_deleteNewCustomShow = false;
}

void KPrEditCustomSlideShowsCommand::undo()
{
    m_model->updateCustomShow(m_name, m_oldCustomShow);
    m_deleteNewCustomShow = true;
}
