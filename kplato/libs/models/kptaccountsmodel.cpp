/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <kplato@kde.org>

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

#include "kptaccountsmodel.h"

#include "kptglobal.h"
#include "kptcommand.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptaccount.h"
#include "kptdatetime.h"
#include "kptschedule.h"

#include <QList>
#include <QObject>


#include <kglobal.h>
#include <klocale.h>

#include <kdebug.h>

namespace KPlato
{

//--------------------------------------
AccountModel::AccountModel()
    : QObject()
{
}

const QMetaEnum AccountModel::columnMap() const
{
    return metaObject()->enumerator( metaObject()->indexOfEnumerator("Properties") );
}

QVariant AccountModel::data( const Account *a, int property, int role ) const
{
    QVariant result;
    if ( a == 0 ) {
        return QVariant();
    }
    switch ( property ) {
        case AccountModel::Name: result = name( a, role ); break;
        case AccountModel::Description: result = description( a, role ); break;
        default:
            kDebug()<<"data: invalid display value column"<<property;
            return QVariant();
    }
    if ( result.isValid() ) {
        if ( role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = " ";
        }
        return result;
    }
    // define default action

    return QVariant();
}

QVariant AccountModel::name( const Account *a, int role ) const
{
    //kDebug()<<res->name()<<","<<role;
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return a->name();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant AccountModel::description( const Account *a, int role ) const
{
    //kDebug()<<res->name()<<","<<role;
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return a->description();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant AccountModel::headerData( int property, int role ) const
{
    if ( role == Qt::DisplayRole ) {
        switch ( property ) {
            case AccountModel::Name: return i18n( "Name" );
            case AccountModel::Description: return i18n( "Description" );
            default: return QVariant();
        }
    }
    if ( role == Qt::TextAlignmentRole ) {
        switch (property) {
            case AccountModel::Description: return Qt::AlignLeft;
            default: return QVariant();
        }
    }
    if ( role == Qt::ToolTipRole ) {
        switch ( property ) {
            case AccountModel::Name: return ToolTip::accountName();
            case AccountModel::Description: return ToolTip::accountDescription();
            default: return QVariant();
        }
    }
    return QVariant();
}

//----------------------------------------
AccountItemModel::AccountItemModel( QObject *parent )
    : ItemModelBase( parent ),
    m_account( 0 )
{
}

AccountItemModel::~AccountItemModel()
{
}

void AccountItemModel::slotAccountToBeInserted( const Account *parent, int row )
{
    //kDebug()<<parent->name();
    Q_ASSERT( m_account == 0 );
    m_account = const_cast<Account*>(parent);
    beginInsertRows( index( parent ), row, row );
}

void AccountItemModel::slotAccountInserted( const Account *account )
{
    //kDebug()<<account->name();
    Q_ASSERT( account->parent() == m_account );
    endInsertRows();
    m_account = 0;
}

void AccountItemModel::slotAccountToBeRemoved( const Account *account )
{
    //kDebug()<<account->name();
    Q_ASSERT( m_account == 0 );
    m_account = const_cast<Account*>(account);
    int row = index( account ).row();
    beginRemoveRows( index( account->parent() ), row, row );
}

void AccountItemModel::slotAccountRemoved( const Account *account )
{
    //kDebug()<<account->name();
    Q_ASSERT( account == m_account );
    endRemoveRows();
    m_account = 0;
}

void AccountItemModel::setProject( Project *project )
{
    if ( m_project ) {
        Accounts *acc = &( m_project->accounts() );
        disconnect( acc , SIGNAL( changed( Account* ) ), this, SLOT( slotAccountChanged( Account* ) ) );
        
        disconnect( acc, SIGNAL( accountAdded( const Account* ) ), this, SLOT( slotAccountInserted( const Account* ) ) );
        disconnect( acc, SIGNAL( accountToBeAdded( const Account*, int ) ), this, SLOT( slotAccountToBeInserted( const Account*, int ) ) );
        
        disconnect( acc, SIGNAL( accountRemoved( const Account* ) ), this, SLOT( slotAccountRemoved( const Account* ) ) );
        disconnect( acc, SIGNAL( accountToBeRemoved( const Account* ) ), this, SLOT( slotAccountToBeRemoved( const Account* ) ) );
    }
    m_project = project;
    if ( project ) {
        Accounts *acc = &( project->accounts() );
        kDebug()<<acc;
        connect( acc, SIGNAL( changed( Account* ) ), this, SLOT( slotAccountChanged( Account* ) ) );
        
        connect( acc, SIGNAL( accountAdded( const Account* ) ), this, SLOT( slotAccountInserted( const Account* ) ) );
        connect( acc, SIGNAL( accountToBeAdded( const Account*, int ) ), this, SLOT( slotAccountToBeInserted( const Account*, int ) ) );
        
        connect( acc, SIGNAL( accountRemoved( const Account* ) ), this, SLOT( slotAccountRemoved( const Account* ) ) );
        connect( acc, SIGNAL( accountToBeRemoved( const Account* ) ), this, SLOT( slotAccountToBeRemoved( const Account* ) ) );
    }
}

Qt::ItemFlags AccountItemModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags flags = ItemModelBase::flags( index );
    if ( !m_readWrite ) {
        return flags &= ~Qt::ItemIsEditable;
    }
    if ( !index.isValid() ) {
        return flags;
    }

    if ( !index.isValid() )
        return flags;
    if ( !m_readWrite ) {
        return flags &= ~Qt::ItemIsEditable;
    }
    if ( account ( index ) ) {
        switch ( index.column() ) {
            default: flags |= Qt::ItemIsEditable;
        }
    }
    return flags;
}


QModelIndex AccountItemModel::parent( const QModelIndex &index ) const
{
    if ( !index.isValid() || m_project == 0 ) {
        return QModelIndex();
    }
    //kDebug()<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    Account *a = account( index );
    if ( a == 0 ) {
        return QModelIndex();
    }
    Account *par = a->parent();
    if ( par ) {
        a = par->parent();
        int row = -1;
        if ( a ) {
            row = a->accountList().indexOf( par );
        } else {
            row = m_project->accounts().accountList().indexOf( par );
        }
        //kDebug()<<par->name()<<":"<<row;
        return createIndex( row, 0, par );
    }
    return QModelIndex();
}

QModelIndex AccountItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( m_project == 0 || column < 0 || column >= columnCount() || row < 0 ) {
        return QModelIndex();
    }
    Account *par = account( parent );
    if ( par == 0 ) {
        if ( row < m_project->accounts().accountList().count() ) {
            return createIndex( row, column, m_project->accounts().accountList().at( row ) );
        }
    } else if ( row < par->accountList().count() ) {
        return createIndex( row, column, par->accountList().at( row ) );
    }
    return QModelIndex();
}

QModelIndex AccountItemModel::index( const Account *account ) const
{
    Account *a = const_cast<Account*>(account);
    if ( m_project == 0 || account == 0 ) {
        return QModelIndex();
    }
    int row = -1;
    Account *par = a->parent();
    if ( par == 0 ) {
         row = m_project->accounts().accountList().indexOf( a );
    } else {
        row = par->accountList().indexOf( a );
    }
    if ( row == -1 ) {
        return QModelIndex();
    }
    return createIndex( row, 0, a );

}

int AccountItemModel::columnCount( const QModelIndex & ) const
{
    return 2;
}

int AccountItemModel::rowCount( const QModelIndex &parent ) const
{
    if ( m_project == 0 ) {
        return 0;
    }
    Account *par = account( parent );
    if ( par == 0 ) {
        return m_project->accounts().accountList().count();
    }
    return par->accountList().count();
}

bool AccountItemModel::setName( Account *a, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() != a->name() ) {
                emit executeCommand( new RenameAccountCmd( a, value.toString(), "Modify account name" ) );
            }
            return true;
    }
    return false;
}

bool AccountItemModel::setDescription( Account *a, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() != a->description() ) {
                emit executeCommand( new ModifyAccountDescriptionCmd( a, value.toString(), "Modify account description" ) );
            }
            return true;
    }
    return false;
}

QVariant AccountItemModel::data( const QModelIndex &index, int role ) const
{
    QVariant result;
    Account *a = account( index );
    if ( a == 0 ) {
        return QVariant();
    }
    result = m_model.data( a, index.column(), role );
    if ( result.isValid() ) {
        if ( role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = " ";
        }
    }
    return result;
}

bool AccountItemModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if ( ( flags( index ) &Qt::ItemIsEditable ) == 0 || role != Qt::EditRole ) {
        return false;
    }
    Account *a = account( index );
    switch (index.column()) {
        case AccountModel::Name: return setName( a, value, role );
        case AccountModel::Description: return setDescription( a, value, role );
        default:
            qWarning("data: invalid display value column %d", index.column());
            return false;
    }
    return false;
}

QVariant AccountItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        return m_model.headerData( section, role );
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Account *AccountItemModel::account( const QModelIndex &index ) const
{
    return static_cast<Account*>( index.internalPointer() );
}

void AccountItemModel::slotAccountChanged( Account *account )
{
    Account *par = account->parent();
    if ( par ) {
        int row = par->accountList().indexOf( account );
        emit dataChanged( createIndex( row, 0, account ), createIndex( row, columnCount() - 1, account ) );
    } else {
        int row = m_project->accounts().accountList().indexOf( account );
        emit dataChanged( createIndex( row, 0, account ), createIndex( row, columnCount() - 1, account ) );
    }
}

QModelIndex AccountItemModel::insertAccount( Account *account, Account *parent )
{
    kDebug();
    if ( account->name().isEmpty() || m_project->accounts().findAccount( account->name() ) ) {
        QString s = parent == 0 ? account->name() : parent->name();
        account->setName( m_project->accounts().uniqueId( s ) );
        //m_project->accounts().insertId( account );
    }
    emit executeCommand( new AddAccountCmd( *m_project, account, parent, i18n( "Add account" ) ) );
    int row = -1;
    if ( parent ) {
        row = parent->accountList().indexOf( account );
    } else {
        row = m_project->accounts().accountList().indexOf( account );
    }
    if ( row != -1 ) {
        //kDebug()<<"Inserted:"<<account->name();
        return createIndex( row, 0, account );
    }
    kDebug()<<"Can't find"<<account->name();
    return QModelIndex();
}

void AccountItemModel::removeAccounts( QList<Account*> lst )
{
    MacroCommand *cmd = 0;
    QString c = lst.count() > 1 ? i18n( "Delete Accounts" ) : i18n( "Delete Account" );
    while ( ! lst.isEmpty() ) {
        bool del = true;
        Account *acc = lst.takeFirst();
        foreach ( Account *a, lst ) {
            if ( acc->isChildOf( a ) ) {
                del = false; // acc will be deleted when a is deleted
                break;
            }
        }
        if ( del ) {
            if ( cmd == 0 ) cmd = new MacroCommand( c );
            cmd->addCommand( new RemoveAccountCmd( *m_project, acc ) );
        }
    }
    if ( cmd )
        emit executeCommand( cmd );
}

//----------------------------------------
CostBreakdownItemModel::CostBreakdownItemModel( QObject *parent )
    : ItemModelBase( parent ),
    m_project( 0 ),
    m_manager( 0 ),
    m_cumulative( false ),
    m_periodtype( Period_Day ),
    m_startmode( StartMode_Project ),
    m_endmode( EndMode_Project )
{
}

CostBreakdownItemModel::~CostBreakdownItemModel()
{
}

void CostBreakdownItemModel::slotAccountToBeInserted( const Account *parent, int row )
{
    //kDebug()<<parent->name();
    beginInsertRows( index( parent ), row, row );
}

void CostBreakdownItemModel::slotAccountInserted( const Account *account )
{
    //kDebug()<<account->name();
    endInsertRows();
}

void CostBreakdownItemModel::slotAccountToBeRemoved( const Account *account )
{
    //kDebug()<<account->name();
    int row = index( account ).row();
    beginRemoveRows( index( account->parent() ), row, row );
}

void CostBreakdownItemModel::slotAccountRemoved( const Account *account )
{
    //kDebug()<<account->name();
    endRemoveRows();
}

void CostBreakdownItemModel::setProject( Project *project )
{
    if ( m_project ) {
        Accounts *acc = &( m_project->accounts() );
        disconnect( acc , SIGNAL( changed( Account* ) ), this, SLOT( slotAccountChanged( Account* ) ) );
        
        disconnect( acc, SIGNAL( accountAdded( const Account* ) ), this, SLOT( slotAccountInserted( const Account* ) ) );
        disconnect( acc, SIGNAL( accountToBeAdded( const Account*, int ) ), this, SLOT( slotAccountToBeInserted( const Account*, int ) ) );
        
        disconnect( acc, SIGNAL( accountRemoved( const Account* ) ), this, SLOT( slotAccountRemoved( const Account* ) ) );
        disconnect( acc, SIGNAL( accountToBeRemoved( const Account* ) ), this, SLOT( slotAccountToBeRemoved( const Account* ) ) );
    }
    m_project = project;
    if ( project ) {
        Accounts *acc = &( project->accounts() );
        kDebug()<<acc;
        connect( acc, SIGNAL( changed( Account* ) ), this, SLOT( slotAccountChanged( Account* ) ) );
        
        connect( acc, SIGNAL( accountAdded( const Account* ) ), this, SLOT( slotAccountInserted( const Account* ) ) );
        connect( acc, SIGNAL( accountToBeAdded( const Account*, int ) ), this, SLOT( slotAccountToBeInserted( const Account*, int ) ) );
        
        connect( acc, SIGNAL( accountRemoved( const Account* ) ), this, SLOT( slotAccountRemoved( const Account* ) ) );
        connect( acc, SIGNAL( accountToBeRemoved( const Account* ) ), this, SLOT( slotAccountToBeRemoved( const Account* ) ) );
    }
}

void CostBreakdownItemModel::setScheduleManager( ScheduleManager *sm )
{
    kDebug()<<m_project<<m_manager<<sm;
    if ( m_manager != sm ) {
        m_manager = sm;
        fetchData();
        updateDates();
        reset();
    }
}

void CostBreakdownItemModel::updateDates()
{
    setStartDate();
    setEndDate();
    kDebug()<<m_startmode<<m_start<<m_endmode<<m_end;
}

long CostBreakdownItemModel::id() const
{
    return m_manager == 0 ? -1 : m_manager->id();
}

void CostBreakdownItemModel::fetchData()
{
    //kDebug()<<m_start<<m_end;
    m_plannedCostMap.clear();
    if ( m_project == 0 || m_manager == 0 ) {
        return;
    }
    const Accounts &accounts = m_project->accounts();
    foreach ( Account *a, accounts.allAccounts() ) {
        if ( a->isElement() ) {
            m_plannedCostMap.insert( a, a->plannedCost( id() ) );
        }
    }
    bool finished = false;
    while ( ! finished ) {
        finished = true;
        foreach ( Account *a, m_plannedCostMap.keys() ) {
            if ( a->parent() && ! m_plannedCostMap.contains( a->parent() ) ) {
                EffortCostMap ec;
                foreach ( Account *ac, a->accountList() ) {
                    ec += m_plannedCostMap.value( ac );
                }
                m_plannedCostMap.insert( a->parent(), ec );
                finished = false;
            }
        }
    }
    //kDebug()<<m_plannedCostMap.keys();
}

QModelIndex CostBreakdownItemModel::parent( const QModelIndex &index ) const
{
    if ( !index.isValid() || m_project == 0 || m_manager == 0 ) {
        return QModelIndex();
    }
    //kDebug()<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    Account *a = account( index );
    if ( a == 0 ) {
        return QModelIndex();
    }
    Account *par = a->parent();
    if ( par ) {
        a = par->parent();
        int row = -1;
        if ( a ) {
            row = a->accountList().indexOf( par );
        } else {
            row = m_project->accounts().accountList().indexOf( par );
        }
        //kDebug()<<par->name()<<":"<<row;
        return createIndex( row, 0, par );
    }
    return QModelIndex();
}

QModelIndex CostBreakdownItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( m_project == 0 || column < 0 || column >= columnCount() || row < 0 ) {
        return QModelIndex();
    }
    Account *par = account( parent );
    if ( par == 0 ) {
        if ( row < m_project->accounts().accountList().count() ) {
            return createIndex( row, column, m_project->accounts().accountList().at( row ) );
        }
    } else if ( row < par->accountList().count() ) {
        return createIndex( row, column, par->accountList().at( row ) );
    }
    return QModelIndex();
}

QModelIndex CostBreakdownItemModel::index( const Account *account ) const
{
    Account *a = const_cast<Account*>(account);
    if ( m_project == 0 || m_manager == 0 || account == 0 ) {
        return QModelIndex();
    }
    int row = -1;
    Account *par = a->parent();
    if ( par == 0 ) {
        row = m_project->accounts().accountList().indexOf( a );
    } else {
        row = par->accountList().indexOf( a );
    }
    if ( row == -1 ) {
        return QModelIndex();
    }
    return createIndex( row, 0, a );

}

int CostBreakdownItemModel::columnCount( const QModelIndex & ) const
{
    int c = 3;
    if ( m_start.isValid() && m_end.isValid() ) {
        switch ( m_periodtype ) {
            case Period_Day: {
                c += m_start.daysTo( m_end ) + 1;
                break;
            }
            case Period_Week: {
                int days = KGlobal::locale()->weekStartDay() - m_start.dayOfWeek();
                if ( days > 0 ) {
                    days -= 7;
                }
                QDate start = m_start.addDays( days );
                c += (start.daysTo( m_end ) / 7) + 1;
                break;
            }
            case Period_Month: {
                break;
            }
        }
    }
    return c;
}

int CostBreakdownItemModel::rowCount( const QModelIndex &parent ) const
{
    if ( m_project == 0 || m_manager == 0 ) {
        return 0;
    }
    Account *par = account( parent );
    if ( par == 0 ) {
        return m_project->accounts().accountList().count();
    }
    return par->accountList().count();
}

QString CostBreakdownItemModel::formatMoney( double cost ) const
{
    return KGlobal::locale()->formatMoney( cost, "", 0 );
}

QVariant CostBreakdownItemModel::data( const QModelIndex &index, int role ) const
{
    QVariant result;
    Account *a = account( index );
    if ( a == 0 ) {
        return QVariant();
    }
    if ( role == Qt::DisplayRole ) {
        switch ( index.column() ) {
            case 0: return a->name();
            case 1: return a->description();
            case 2: return formatMoney( m_plannedCostMap.value( a ).totalCost() );
            default: {
                int col = index.column() - 3;
                EffortCostMap ec = m_plannedCostMap.value( a );
                switch ( m_periodtype ) {
                    case Period_Day: {
                        double cost = 0.0;
                        if ( m_cumulative ) {
                            cost = ec.costTo( m_start.addDays( col ) );
                        } else {
                            cost = ec.costOnDate( m_start.addDays( col ) );
                        }
                        //kDebug()<<m_start<<m_start.addDays( col )<<cost;
                        return cost;
                    }
                    case Period_Week: {
                        double cost = 0.0; 
                        int days = KGlobal::locale()->weekStartDay() - m_start.dayOfWeek();
                        if ( days > 0 ) {
                            days -= 7; ;
                        }
                        QDate start = m_start.addDays( days );
                        int week = col;
                        if ( m_cumulative ) {
                            cost = ec.costTo( start.addDays( ++week * 7 ) );
                        } else {
                            cost = week == 0 ? ec.cost( m_start, m_start.daysTo( start.addDays( 7 ) ) ) : ec.cost( start.addDays( week * 7 ) );
                        }
                        return formatMoney( cost );
                    }
                    case Period_Month:
                    default:
                        return 0.0;
                        break;
                }
            }
        }
    }
    if ( role == Qt::TextAlignmentRole ) {
        return headerData( index.column(), Qt::Horizontal, role );
    }
    return QVariant();
}

int CostBreakdownItemModel::periodType() const
{
    return m_periodtype;
}

void CostBreakdownItemModel::setPeriodType( int period )
{
    if ( m_periodtype != period ) {
        m_periodtype = period;
        reset();
    }
}

int CostBreakdownItemModel::startMode() const
{
    return m_startmode;
}

void CostBreakdownItemModel::setStartMode( int mode )
{
    if ( m_startmode != mode ) {
        m_startmode = mode;
        setStartDate();
        reset();
    }
}

int CostBreakdownItemModel::endMode() const
{
    return m_endmode;
}

void CostBreakdownItemModel::setEndMode( int mode )
{
    if ( m_endmode != mode ) {
        m_endmode = mode;
        setEndDate();
        reset();
    }
}

void CostBreakdownItemModel::setStartDate()
{
    if ( m_startmode == StartMode_Project ) {
        m_start = m_project->startTime( id() ).date();
        foreach ( const EffortCostMap &ec, m_plannedCostMap.values() ) {
            if ( ! ec.startDate().isValid() ) {
                continue;
            }
            if ( ! m_start.isValid() || m_start > ec.startDate() ) {
                m_start = ec.startDate();
            }
        }
    }
}

void CostBreakdownItemModel::setEndDate()
{
    if ( m_endmode == EndMode_Project ) {
        m_end = m_project->endTime( id() ).date();
        foreach ( const EffortCostMap &ec, m_plannedCostMap.values() ) {
            if ( ! ec.endDate().isValid() ) {
                continue;
            }
            if ( ! m_end.isValid() || m_end < ec.endDate() ) {
                m_end = ec.endDate();
            }
        }
    } else if ( m_endmode == EndMode_CurrentDate ) {
        m_end = QDate::currentDate();
    }
}

QDate CostBreakdownItemModel::startDate() const
{
    return m_start;
}


void CostBreakdownItemModel::setStartDate( const QDate &date )
{
    m_start = date;
    reset();
}

QDate CostBreakdownItemModel::endDate() const
{
    return m_end;
}

void CostBreakdownItemModel::setEndDate( const QDate &date )
{
    m_end = date;
    reset();
}

bool CostBreakdownItemModel::cumulative() const
{
    return m_cumulative;
}

void CostBreakdownItemModel::setCumulative( bool on )
{
    m_cumulative = on;
    reset();
}

QVariant CostBreakdownItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        if ( role == Qt::DisplayRole ) {
            if ( section == 0 ) {
                return i18n( "Name" );
            }
            if ( section == 1 ) {
                return i18n( "Description" );
            }
            if ( section == 2 ) {
                return i18n( "Total" );
            }
            switch ( m_periodtype ) {
                case Period_Day: {
                    return m_start.addDays( section - 3 ).toString( Qt::ISODate );
                }
                case Period_Week: {
                    return m_start.addDays( ( section - 3 ) * 7 ).weekNumber();
                }
                case Period_Month:
                default:
                    return section;
                    break;
            }
            return QVariant();
        }
        if ( role == Qt::ToolTipRole ) {
            switch ( section ) {
                case 0: return ToolTip::accountName();
                case 1: return ToolTip::accountDescription();
                case 2: return i18n( "The total cost for the account" );
                default: return QVariant();
            }
        }
        if ( role == Qt::TextAlignmentRole ) {
            switch ( section ) {
                case 0: return QVariant();
                case 1: return QVariant();
                default: return Qt::AlignRight;
            }
            return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Account *CostBreakdownItemModel::account( const QModelIndex &index ) const
{
    return static_cast<Account*>( index.internalPointer() );
}

void CostBreakdownItemModel::slotAccountChanged( Account *account )
{
    Account *par = account->parent();
    if ( par ) {
        int row = par->accountList().indexOf( account );
        emit dataChanged( createIndex( row, 0, account ), createIndex( row, columnCount() - 1, account ) );
    } else {
        int row = m_project->accounts().accountList().indexOf( account );
        emit dataChanged( createIndex( row, 0, account ), createIndex( row, columnCount() - 1, account ) );
    }
}


} // namespace KPlato

#include "kptaccountsmodel.moc"
