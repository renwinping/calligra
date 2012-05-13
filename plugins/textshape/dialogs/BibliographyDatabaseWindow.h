/* This file is part of the KDE project
 * Copyright (C) 2011 Smit Patel <smitpatel24@gmail.com>
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

#ifndef BIBLIOGRAPHYDATABASEWINDOW_H
#define BIBLIOGRAPHYDATABASEWINDOW_H

#include "ui_BibliographyDatabaseWindow.h"

#include <QMainWindow>
#include <QTableView>
#include <QDir>

class BibliographyDb;
class QTableView;
class QDir;

class BibliographyDatabaseWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit BibliographyDatabaseWindow(QWidget *parent = 0);
    ~BibliographyDatabaseWindow();
    void loadBibliographyDbs();
    
private:
    Ui::BibliographyDatabaseWindow ui;
    QTableView *m_bibTableView;
    BibliographyDb *m_table;

public:
    QDir tableDir;

private slots:
    void tableChanged(QString);
    void insertBlankRow();
};

#endif // BIBLIOGRAPHYDATABASEWINDOW_H
