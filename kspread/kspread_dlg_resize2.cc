/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 1999, 2000 Montel Laurent <montell@club-internet.fr>
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

#include <qprinter.h>

#include "kspread_dlg_resize2.h"
#include "kspread_view.h"
#include "kspread_canvas.h"
#include "kspread_util.h"
#include "kspread_layout.h"
#include "kspread_table.h"
#include <qlayout.h>
#include <kapp.h>
#include <klocale.h>
#include <kbuttonbox.h>
//#include <iostream.h>
#include <kdebug.h>

KSpreadresize2::KSpreadresize2( KSpreadView* parent, const char* name,type_resize re)
	: QDialog( parent, name,TRUE )
{

  m_pView=parent;
  type=re;
  QString tmp;
  QString tmpCheck;
  int pos;
  QString label;
  QVBoxLayout *lay1 = new QVBoxLayout( this );
  lay1->setMargin( 5 );
  lay1->setSpacing( 10 );
  tmpCheck=i18n("Default");
  RowLayout *rl;
  ColumnLayout *cl;
  switch(type)
	{
	case resize_row:
		setCaption( i18n("Resize row") );
//		size=rl->height(m_pView->canvasWidget());
		size=20;
		label=i18n("Height");
		tmpCheck+=" (20)";
		break;
	case resize_column:
		setCaption( i18n("Resize column") );
		//size=cl->width(m_pView->canvasWidget());
		size=60;
		label=i18n("Width");
		tmpCheck+=" (60)";
		break;
	default :
	        kdDebug(36001) <<"Err in type_resize" << endl;
		break;
	}

  

  m_pSize2=new KIntNumInput(size, this, 10);
  m_pSize2->setRange(20, 400, 1);
  m_pSize2->setLabel(label);
  lay1->addWidget(m_pSize2);
  
  m_pDefault=new QCheckBox(tmpCheck,this);
  lay1->addWidget(m_pDefault);
  
  KButtonBox *bb = new KButtonBox( this );
  bb->addStretch();
  m_pOk = bb->addButton( i18n("OK") );
  m_pOk->setDefault( TRUE );
  m_pClose = bb->addButton( i18n( "Close" ) );
  bb->layout();
  lay1->addWidget( bb );
  lay1->activate();
  m_pSize2->setFocus();
  connect( m_pClose, SIGNAL( clicked() ), this, SLOT( slotClose() ) );
  connect( m_pOk, SIGNAL( clicked() ), this, SLOT( slotOk() ) );

}

void KSpreadresize2::slotOk()
{
QRect selection( m_pView->activeTable()->selectionRect() ); 
int new_size=m_pSize2->value();
if(m_pDefault->isChecked())
{
switch(type)
	{
	case resize_row:
		if(size!=20)
			for(int i=selection.top();i<=selection.bottom();i++)
				m_pView->vBorderWidget()->resizeRow(20,i );
		break;
	case resize_column:
		if(size!=60)
			for(int i=selection.left();i<=selection.right();i++)
				m_pView->hBorderWidget()->resizeColumn(60,i );
		break;
	default :
	        kdDebug(36001) <<"Err in type_resize" << endl;
		break;
	}
}
else
{
if(new_size!=size)
{
switch(type)
	{
	case resize_row:
		if(m_pDefault->isChecked())
			for(int i=selection.top();i<=selection.bottom();i++)
					m_pView->vBorderWidget()->resizeRow(20,i );
		else
			for(int i=selection.top();i<=selection.bottom();i++)
				m_pView->vBorderWidget()->resizeRow(new_size,i );
		break;
	case resize_column:
		if(m_pDefault->isChecked())
			for(int i=selection.left();i<=selection.right();i++)
				m_pView->hBorderWidget()->resizeColumn(60,i );
		else
			for(int i=selection.left();i<=selection.right();i++)
				m_pView->hBorderWidget()->resizeColumn(new_size,i );
		break;
	default :
	        kdDebug(36001) <<"Err in type_resize" << endl;
		break;
	}
}
}
accept();
}

void KSpreadresize2::slotClose()
{

reject();
}


#include "kspread_dlg_resize2.moc"
