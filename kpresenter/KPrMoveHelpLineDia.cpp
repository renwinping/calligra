// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C)  2002 Montel Laurent <lmontel@mandrakesoft.com>

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

#include <klocale.h>
#include <q3vbox.h>
#include <QLayout>
#include <QLabel>
#include <knuminput.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <KoUnit.h>
#include <klineedit.h>
#include <knumvalidator.h>
#include <KoUnitWidgets.h>
#include <khbox.h>
#include "KPrMoveHelpLineDia.h"
#include "KPrDocument.h"
#include <QVBoxLayout>

KPrMoveHelpLineDia::KPrMoveHelpLineDia( QWidget *parent, double value, double limitTop, double limitBottom,
                                        KPrDocument *_doc, const char *name)
    : KDialog( parent )
{
    setButtons(Ok | Cancel | User1);
    showButtonSeparator(true);

    m_doc=_doc;
    m_bRemoveLine = false;

    setButtonText( KDialog::User1, i18n("Remove") );
    setCaption( i18n("Change Help Line Position") );
    QWidget *page = mainWidget();
    QLabel* label = new QLabel(i18n("Position:"), page);
    position= new KoUnitDoubleSpinBox( page, qMax(0.00, limitTop), qMax(0.00, limitBottom), 1, qMax(0.00, value));
    position->setUnit(m_doc->unit() );
    QVBoxLayout* layout = new QVBoxLayout( page );
    layout->addWidget( label );
    layout->addWidget( position );

    connect( this, SIGNAL( user1Clicked() ), this ,SLOT( slotRemoveHelpLine() ));
    resize( 300,100 );
}

void KPrMoveHelpLineDia::slotRemoveHelpLine()
{
    m_bRemoveLine = true;
    accept();
}

double KPrMoveHelpLineDia::newPosition() const
{
    return position->value();
}


KPrInsertHelpLineDia::KPrInsertHelpLineDia( QWidget *parent, const KoRect & _pageRect,
                                            KPrDocument *_doc, const char *name)
    : KDialog( parent )
{
    setButtons(Ok|Cancel);
    showButtonSeparator(true);

    limitOfPage=_pageRect;
    m_doc=_doc;
    setCaption( i18n("Add New Help Line") );
    QWidget *page = mainWidget();
    Q3ButtonGroup *group = new Q3ButtonGroup( 1, Qt::Horizontal,i18n("Orientation"), page );
    group->setRadioButtonExclusive( true );
    group->layout();
    m_rbHoriz = new QRadioButton( i18n("Horizontal"), group );
    m_rbVert = new QRadioButton( i18n("Vertical"), group );

    connect( group , SIGNAL( clicked( int) ), this, SLOT( slotRadioButtonClicked() ));

    QLabel* label = new QLabel(i18n("Position:"), page);

    position= new KoUnitDoubleSpinBox( page,qMax(0.00, limitOfPage.top() ), qMax(0.00, limitOfPage.bottom()),1,0.00 );

    position->setUnit( m_doc->unit() );
    m_rbHoriz->setChecked( true );

    QVBoxLayout* layout = new QVBoxLayout( page );
    layout->addWidget(label);
    layout->addWidget(position);

    resize( 300,100 );
}

double KPrInsertHelpLineDia::newPosition() const
{
    return position->value();
}

bool KPrInsertHelpLineDia::addHorizontalHelpLine()
{
    return m_rbHoriz->isChecked();
}

void KPrInsertHelpLineDia::slotRadioButtonClicked()
{
    if ( m_rbHoriz->isChecked() )
    {
        position->setMinValue( qMax(0.00, limitOfPage.top() ) );
        position->setMaxValue( qMax(0.00, limitOfPage.bottom() ) );
    }
    else if ( m_rbVert->isChecked() )
    {
        position->setMinValue( qMax(0.00, limitOfPage.left()) );
        position->setMaxValue( qMax(0.00, limitOfPage.right()) );
    }
}

KPrInsertHelpPointDia::KPrInsertHelpPointDia( QWidget *parent, const KoRect & _pageRect,
                                              KPrDocument *_doc, double posX, double posY, const char *name)
    : KDialog( parent ),
      m_bRemovePoint( false )
{
    setButtons(Ok|Cancel| User1);
    showButtonSeparator(true);
    limitOfPage=_pageRect;
    m_doc=_doc;
    setButtonText( KDialog::User1, i18n("Remove") );
    setCaption( i18n("Add New Help Point") );
    QWidget *page = mainWidget();
    QLabel *lab=new QLabel(i18n("X position:"), page);
    positionX= new KoUnitDoubleSpinBox( page, qMax(0.00, limitOfPage.left()),qMax(0.00, limitOfPage.right()),1,qMax(0.00, posX) ) ;
    positionX->setUnit( m_doc->unit() );
    QVBoxLayout* layout = new QVBoxLayout( page );
    layout->addWidget(lab);
    layout->addWidget(positionX);


    lab=new QLabel(i18n("Y position:"), page);
    positionY= new KoUnitDoubleSpinBox( page, qMax(0.00, limitOfPage.top()),qMax(0.00, limitOfPage.bottom()),1,  qMax(0.00, posY) );
    positionY->setUnit( m_doc->unit() );
    layout->addWidget(lab);
    layout->addWidget(positionY);

    showButton( KDialog::User1, (posX!=0.0 || posY!=0.0) );

    connect( this, SIGNAL( user1Clicked() ), this ,SLOT( slotRemoveHelpPoint() ));

    resize( 300,100 );
}

KoPoint KPrInsertHelpPointDia::newPosition() const
{
    return KoPoint( positionX->value(),
                    positionY->value() );
}

void KPrInsertHelpPointDia::slotRemoveHelpPoint()
{
    m_bRemovePoint = true;
    accept();
}

#include "KPrMoveHelpLineDia.moc"
