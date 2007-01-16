/* This file is part of the KDE project
  Copyright (C) 2004 - 2007 Dag Andersen <danders@get2net.dk>

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

#include "kptcommand.h"
#include "kptaccount.h"
#include "kptappointment.h"
#include "kptpart.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptrelation.h"
#include "kptresource.h"

#include <kdebug.h>
#include <klocale.h>

#include <QHash>
#include <QLineEdit>
#include <QMap>

namespace KPlato
{

void NamedCommand::setCommandType( int type )
{
    if ( m_part )
        m_part->setCommandType( type );
}

void NamedCommand::setSchDeleted()
{
    QMap<Schedule*, bool>::Iterator it;
    for ( it = m_schedules.begin(); it != m_schedules.end(); ++it ) {
        kDebug() << k_funcinfo << it.key() ->id() << ": " << it.value() << endl;
        it.key() ->setDeleted( it.value() );
    }
}
void NamedCommand::setSchDeleted( bool state )
{
    QMap<Schedule*, bool>::Iterator it;
    for ( it = m_schedules.begin(); it != m_schedules.end(); ++it ) {
        kDebug() << k_funcinfo << it.key() ->id() << ": " << state << endl;
        it.key() ->setDeleted( state );
    }
}
void NamedCommand::setSchScheduled()
{
    QMap<Schedule*, bool>::Iterator it;
    for ( it = m_schedules.begin(); it != m_schedules.end(); ++it ) {
        kDebug() << k_funcinfo << it.key() ->id() << ": " << it.value() << endl;
        it.key() ->setScheduled( it.value() );
    }
}
void NamedCommand::setSchScheduled( bool state )
{
    QMap<Schedule*, bool>::Iterator it;
    for ( it = m_schedules.begin(); it != m_schedules.end(); ++it ) {
        kDebug() << k_funcinfo << it.key() ->id() << ": " << state << endl;
        it.key() ->setScheduled( state );
    }
}
void NamedCommand::addSchScheduled( Schedule *sch )
{
    kDebug() << k_funcinfo << sch->id() << ": " << sch->isScheduled() << endl;
    m_schedules.insert( sch, sch->isScheduled() );
    foreach ( Appointment * a, sch->appointments() ) {
        if ( a->node() == sch ) {
            m_schedules.insert( a->resource(), a->resource() ->isScheduled() );
        } else if ( a->resource() == sch ) {
            m_schedules.insert( a->node(), a->node() ->isScheduled() );
        }
    }
}
void NamedCommand::addSchDeleted( Schedule *sch )
{
    kDebug() << k_funcinfo << sch->id() << ": " << sch->isDeleted() << endl;
    m_schedules.insert( sch, sch->isDeleted() );
    foreach ( Appointment * a, sch->appointments() ) {
        if ( a->node() == sch ) {
            m_schedules.insert( a->resource(), a->resource() ->isDeleted() );
        } else if ( a->resource() == sch ) {
            m_schedules.insert( a->node(), a->node() ->isDeleted() );
        }
    }
}

//-------------------------------------------------
CalendarAddCmd::CalendarAddCmd( Part *part, Project *project, Calendar *cal, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_cal( cal ),
        m_added( false )
{
    cal->setDeleted( true );
    //kDebug()<<k_funcinfo<<cal->name()<<endl;
}

void CalendarAddCmd::execute()
{
    if ( !m_added && m_project ) {
        m_project->addCalendar( m_cal );
        m_added = true;
    }
    m_cal->setDeleted( false );

    setCommandType( 0 );
    //kDebug()<<k_funcinfo<<m_cal->name()<<" added to: "<<m_project->name()<<endl;
}

void CalendarAddCmd::unexecute()
{
    m_cal->setDeleted( true );

    setCommandType( 0 );
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
}

CalendarDeleteCmd::CalendarDeleteCmd( Part *part, Calendar *cal, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal )
{

    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}

void CalendarDeleteCmd::execute()
{
    m_cal->setDeleted( true );
    setSchScheduled( false );
    setCommandType( 1 );
}

void CalendarDeleteCmd::unexecute()
{
    m_cal->setDeleted( false );
    setSchScheduled();
    setCommandType( 0 );
}

CalendarModifyNameCmd::CalendarModifyNameCmd( Part *part, Calendar *cal, const QString& newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal )
{

    m_oldvalue = cal->name();
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<cal->name()<<endl;
}
void CalendarModifyNameCmd::execute()
{
    m_cal->setName( m_newvalue );
    setCommandType( 0 );
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
}
void CalendarModifyNameCmd::unexecute()
{
    m_cal->setName( m_oldvalue );
    setCommandType( 0 );
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
}

CalendarModifyParentCmd::CalendarModifyParentCmd( Part *part, Calendar *cal, Calendar *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal )
{

    m_oldvalue = cal->parent();
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<cal->name()<<endl;
    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}
void CalendarModifyParentCmd::execute()
{
    m_cal->setParent( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void CalendarModifyParentCmd::unexecute()
{
    m_cal->setParent( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

CalendarAddDayCmd::CalendarAddDayCmd( Part *part, Calendar *cal, CalendarDay *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal ),
        m_mine( true )
{

    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<cal->name()<<endl;
    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}
CalendarAddDayCmd::~CalendarAddDayCmd()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_mine )
        delete m_newvalue;
}
void CalendarAddDayCmd::execute()
{
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
    m_cal->addDay( m_newvalue );
    m_mine = false;
    setSchScheduled( false );
    setCommandType( 1 );
}
void CalendarAddDayCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
    m_cal->takeDay( m_newvalue );
    m_mine = true;
    setSchScheduled();
    setCommandType( 1 );
}

CalendarRemoveDayCmd::CalendarRemoveDayCmd( Part *part, Calendar *cal, const QDate &day, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal ),
        m_mine( false )
{

    m_value = cal->findDay( day );
    //kDebug()<<k_funcinfo<<cal->name()<<endl;
    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}
void CalendarRemoveDayCmd::execute()
{
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
    m_cal->takeDay( m_value );
    m_mine = true;
    setSchScheduled( false );
    setCommandType( 1 );
}
void CalendarRemoveDayCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<m_cal->name()<<endl;
    m_cal->addDay( m_value );
    m_mine = false;
    setSchScheduled();
    setCommandType( 1 );
}

CalendarModifyDayCmd::CalendarModifyDayCmd( Part *part, Calendar *cal, CalendarDay *value, const QString& name )
        : NamedCommand( part, name ),
        m_cal( cal ),
        m_mine( true )
{

    m_newvalue = value;
    m_oldvalue = cal->findDay( value->date() );
    //kDebug()<<k_funcinfo<<cal->name()<<" old:("<<m_oldvalue<<") new:("<<m_newvalue<<")"<<endl;
    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}
CalendarModifyDayCmd::~CalendarModifyDayCmd()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_mine ) {
        delete m_newvalue;
    } else {
        delete m_oldvalue;
    }
}
void CalendarModifyDayCmd::execute()
{
    //kDebug()<<k_funcinfo<<endl;
    m_cal->takeDay( m_oldvalue );
    m_cal->addDay( m_newvalue );
    m_mine = false;
    setSchScheduled( false );
    setCommandType( 1 );
}
void CalendarModifyDayCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<endl;
    m_cal->takeDay( m_newvalue );
    m_cal->addDay( m_oldvalue );
    m_mine = true;
    setSchScheduled();
    setCommandType( 1 );
}

CalendarModifyWeekdayCmd::CalendarModifyWeekdayCmd( Part *part, Calendar *cal, int weekday, CalendarDay *value, const QString& name )
        : NamedCommand( part, name ),
        m_weekday( weekday ),
        m_cal( cal ),
        m_mine( true )
{

    m_value = value;
    kDebug() << k_funcinfo << cal->name() << " (" << value << ")" << endl;
    // TODO check if any resources uses this calendar
    if ( part ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            addSchScheduled( s );
        }
    }
}
CalendarModifyWeekdayCmd::~CalendarModifyWeekdayCmd()
{
    kDebug() << k_funcinfo << m_weekday << ": " << m_value << endl;
    delete m_value;

}
void CalendarModifyWeekdayCmd::execute()
{
    m_value = m_cal->weekdays() ->replace( m_weekday, m_value );
    setSchScheduled( false );
    setCommandType( 1 );
}
void CalendarModifyWeekdayCmd::unexecute()
{
    m_value = m_cal->weekdays() ->replace( m_weekday, m_value );
    setSchScheduled();
    setCommandType( 1 );
}

NodeDeleteCmd::NodeDeleteCmd( Part *part, Node *node, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        m_index( -1 )
{

    m_parent = node->getParent();
    m_mine = false;

    m_project = static_cast<Project*>( node->projectNode() );
    if ( m_project ) {
        foreach ( Schedule * s, part->getProject().schedules() ) {
            if ( s && s->isScheduled() ) {
                // Only invalidate schedules this node is part of
                addSchScheduled( s );
            }
        }
    }
    m_cmd = new KMacroCommand( "" );
    foreach ( Relation * r, node->dependChildNodes() ) {
        m_cmd->addCommand( new DeleteRelationCmd( part, r ) );
    }
    foreach ( Relation * r, node->dependParentNodes() ) {
        m_cmd->addCommand( new DeleteRelationCmd( part, r ) );
    }
    QList<Node*> lst = node->childNodeIterator();
    for ( int i = lst.count(); i > 0; --i ) {
        m_cmd->addCommand( new NodeDeleteCmd( part, lst[ i - 1 ] ) );
    }

}
NodeDeleteCmd::~NodeDeleteCmd()
{
    if ( m_mine )
        delete m_node;
    delete m_cmd;
    while ( !m_appointments.isEmpty() )
        delete m_appointments.takeFirst();
}
void NodeDeleteCmd::execute()
{
    if ( m_parent && m_project ) {
        m_index = m_parent->findChildNode( m_node );
        //kDebug()<<k_funcinfo<<m_node->name()<<" "<<m_index<<endl;
        foreach ( Appointment * a, m_node->appointments() ) {
            a->detach();
            m_appointments.append( a );
        }
        m_cmd->execute();
        m_project->delTask( m_node );
        m_mine = true;
        setSchScheduled( false );
        setCommandType( 1 );
    }
}
void NodeDeleteCmd::unexecute()
{
    if ( m_parent && m_project ) {
        //kDebug()<<k_funcinfo<<m_node->name()<<" "<<m_index<<endl;
        m_project->addSubTask( m_node, m_index, m_parent );
        m_cmd->unexecute();
        while ( !m_appointments.isEmpty() ) {
            m_appointments.takeFirst() ->attach();
        }
        m_mine = false;
        setSchScheduled();
        setCommandType( 1 );
    }
}

TaskAddCmd::TaskAddCmd( Part *part, Project *project, Node *node, Node *after, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_node( node ),
        m_after( after ),
        m_added( false )
{

    // set some reasonable defaults for normally calculated values
    if ( after && after->getParent() && after->getParent() != project ) {
        node->setStartTime( after->getParent() ->startTime() );
        node->setEndTime( node->startTime() + node->duration() );
    } else {
        if ( project->constraint() == Node::MustFinishOn ) {
            node->setEndTime( project->endTime() );
            node->setStartTime( node->endTime() - node->duration() );
        } else {
            node->setStartTime( project->startTime() );
            node->setEndTime( node->startTime() + node->duration() );
        }
    }
    node->setEarliestStart( node->startTime() );
    node->setLatestFinish( node->endTime() );
    node->setWorkStartTime( node->startTime() );
    node->setWorkEndTime( node->endTime() );
}
TaskAddCmd::~TaskAddCmd()
{
    if ( !m_added )
        delete m_node;
}
void TaskAddCmd::execute()
{
    //kDebug()<<k_funcinfo<<m_node->name()<<endl;
    m_project->addTask( m_node, m_after );
    m_added = true;

    setCommandType( 1 );
}
void TaskAddCmd::unexecute()
{
    m_project->delTask( m_node );
    m_added = false;

    setCommandType( 1 );
}

SubtaskAddCmd::SubtaskAddCmd( Part *part, Project *project, Node *node, Node *parent, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_node( node ),
        m_parent( parent ),
        m_added( false )
{

    // set some reasonable defaults for normally calculated values
    node->setStartTime( parent->startTime() );
    node->setEndTime( node->startTime() + node->duration() );
    node->setEarliestStart( node->startTime() );
    node->setLatestFinish( node->endTime() );
    node->setWorkStartTime( node->startTime() );
    node->setWorkEndTime( node->endTime() );
}
SubtaskAddCmd::~SubtaskAddCmd()
{
    if ( !m_added )
        delete m_node;
}
void SubtaskAddCmd::execute()
{
    m_project->addSubTask( m_node, m_parent );
    m_added = true;

    setCommandType( 1 );
}
void SubtaskAddCmd::unexecute()
{
    m_project->delTask( m_node );
    m_added = false;

    setCommandType( 1 );
}

NodeModifyNameCmd::NodeModifyNameCmd( Part *part, Node &node, const QString& nodename, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newName( nodename ),
        oldName( node.name() )
{
}
void NodeModifyNameCmd::execute()
{
    m_node.setName( newName );

    setCommandType( 0 );
}
void NodeModifyNameCmd::unexecute()
{
    m_node.setName( oldName );

    setCommandType( 0 );
}

NodeModifyLeaderCmd::NodeModifyLeaderCmd( Part *part, Node &node, const QString& leader, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newLeader( leader ),
        oldLeader( node.leader() )
{
}
void NodeModifyLeaderCmd::execute()
{
    m_node.setLeader( newLeader );

    setCommandType( 0 );
}
void NodeModifyLeaderCmd::unexecute()
{
    m_node.setLeader( oldLeader );

    setCommandType( 0 );
}

NodeModifyDescriptionCmd::NodeModifyDescriptionCmd( Part *part, Node &node, const QString& description, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newDescription( description ),
        oldDescription( node.description() )
{
}
void NodeModifyDescriptionCmd::execute()
{
    m_node.setDescription( newDescription );

    setCommandType( 0 );
}
void NodeModifyDescriptionCmd::unexecute()
{
    m_node.setDescription( oldDescription );

    setCommandType( 0 );
}

NodeModifyConstraintCmd::NodeModifyConstraintCmd( Part *part, Node &node, Node::ConstraintType c, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newConstraint( c ),
        oldConstraint( static_cast<Node::ConstraintType>( node.constraint() ) )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void NodeModifyConstraintCmd::execute()
{
    m_node.setConstraint( newConstraint );
    setSchScheduled( false );
    setCommandType( 1 );
}
void NodeModifyConstraintCmd::unexecute()
{
    m_node.setConstraint( oldConstraint );
    setSchScheduled();
    setCommandType( 1 );
}

NodeModifyConstraintStartTimeCmd::NodeModifyConstraintStartTimeCmd( Part *part, Node &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.constraintStartTime() )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void NodeModifyConstraintStartTimeCmd::execute()
{
    m_node.setConstraintStartTime( newTime );
    setSchScheduled( false );
    setCommandType( 1 );
}
void NodeModifyConstraintStartTimeCmd::unexecute()
{
    m_node.setConstraintStartTime( oldTime );
    setSchScheduled();
    setCommandType( 1 );
}

NodeModifyConstraintEndTimeCmd::NodeModifyConstraintEndTimeCmd( Part *part, Node &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.constraintEndTime() )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void NodeModifyConstraintEndTimeCmd::execute()
{
    m_node.setConstraintEndTime( newTime );
    setSchScheduled( false );
    setCommandType( 1 );
}
void NodeModifyConstraintEndTimeCmd::unexecute()
{
    m_node.setConstraintEndTime( oldTime );
    setSchScheduled();
    setCommandType( 1 );
}

NodeModifyStartTimeCmd::NodeModifyStartTimeCmd( Part *part, Node &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.startTime() )
{
}
void NodeModifyStartTimeCmd::execute()
{
    m_node.setStartTime( newTime );

    setCommandType( 1 );
}
void NodeModifyStartTimeCmd::unexecute()
{
    m_node.setStartTime( oldTime );

    setCommandType( 1 );
}

NodeModifyEndTimeCmd::NodeModifyEndTimeCmd( Part *part, Node &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.endTime() )
{
}
void NodeModifyEndTimeCmd::execute()
{
    m_node.setEndTime( newTime );

    setCommandType( 1 );
}
void NodeModifyEndTimeCmd::unexecute()
{
    m_node.setEndTime( oldTime );

    setCommandType( 1 );
}

NodeModifyIdCmd::NodeModifyIdCmd( Part *part, Node &node, const QString& id, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newId( id ),
        oldId( node.id() )
{
}
void NodeModifyIdCmd::execute()
{
    m_node.setId( newId );

    setCommandType( 0 );
}
void NodeModifyIdCmd::unexecute()
{
    m_node.setId( oldId );

    setCommandType( 0 );
}

NodeIndentCmd::NodeIndentCmd( Part *part, Node &node, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        m_newparent( 0 ),
        m_newindex( -1 )
{
}
void NodeIndentCmd::execute()
{
    m_oldparent = m_node.getParent();
    m_oldindex = m_oldparent->findChildNode( &m_node );
    Project *p = dynamic_cast<Project *>( m_node.projectNode() );
    if ( p && p->indentTask( &m_node, m_newindex ) ) {
        m_newparent = m_node.getParent();
        m_newindex = m_newparent->findChildNode( &m_node );
    }

    setCommandType( 1 );
}
void NodeIndentCmd::unexecute()
{
    Project * p = dynamic_cast<Project *>( m_node.projectNode() );
    if ( m_newindex != -1 && p && p->unindentTask( &m_node ) ) {
        m_newindex = -1;
    }

    setCommandType( 1 );
}

NodeUnindentCmd::NodeUnindentCmd( Part *part, Node &node, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        m_newparent( 0 ),
        m_newindex( -1 )
{}
void NodeUnindentCmd::execute()
{
    m_oldparent = m_node.getParent();
    m_oldindex = m_oldparent->findChildNode( &m_node );
    Project *p = dynamic_cast<Project *>( m_node.projectNode() );
    if ( p && p->unindentTask( &m_node ) ) {
        m_newparent = m_node.getParent();
        m_newindex = m_newparent->findChildNode( &m_node );
    }

    setCommandType( 1 );
}
void NodeUnindentCmd::unexecute()
{
    Project * p = dynamic_cast<Project *>( m_node.projectNode() );
    if ( m_newindex != -1 && p && p->indentTask( &m_node, m_oldindex ) ) {
        m_newindex = -1;
    }

    setCommandType( 1 );
}

NodeMoveUpCmd::NodeMoveUpCmd( Part *part, Node &node, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        m_moved( false )
{

    m_project = static_cast<Project *>( m_node.projectNode() );
}
void NodeMoveUpCmd::execute()
{
    if ( m_project ) {
        m_moved = m_project->moveTaskUp( &m_node );
    }

    setCommandType( 0 );
}
void NodeMoveUpCmd::unexecute()
{
    if ( m_project && m_moved ) {
        m_project->moveTaskDown( &m_node );
    }
    m_moved = false;
    setCommandType( 0 );
}

NodeMoveDownCmd::NodeMoveDownCmd( Part *part, Node &node, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        m_moved( false )
{

    m_project = static_cast<Project *>( m_node.projectNode() );
}
void NodeMoveDownCmd::execute()
{
    if ( m_project ) {
        m_moved = m_project->moveTaskDown( &m_node );
    }
    setCommandType( 0 );
}
void NodeMoveDownCmd::unexecute()
{
    if ( m_project && m_moved ) {
        m_project->moveTaskUp( &m_node );
    }
    m_moved = false;
    setCommandType( 0 );
}

AddRelationCmd::AddRelationCmd( Part *part, Relation *rel, const QString& name )
        : NamedCommand( part, name ),
        m_rel( rel )
{

    m_taken = true;
    Node *p = rel->parent() ->projectNode();
    if ( p ) {
        foreach ( Schedule * s, p->schedules() ) {
            addSchScheduled( s );
        }
    }
}
AddRelationCmd::~AddRelationCmd()
{
    if ( m_taken )
        delete m_rel;
}
void AddRelationCmd::execute()
{
    //kDebug()<<k_funcinfo<<m_rel->parent()<<" to "<<m_rel->child()<<endl;
    m_taken = false;
    m_rel->parent() ->addDependChildNode( m_rel );
    m_rel->child() ->addDependParentNode( m_rel );
    setSchScheduled( false );
    setCommandType( 1 );
}
void AddRelationCmd::unexecute()
{
    m_taken = true;
    m_rel->parent() ->takeDependChildNode( m_rel );
    m_rel->child() ->takeDependParentNode( m_rel );
    setSchScheduled();
    setCommandType( 1 );
}

DeleteRelationCmd::DeleteRelationCmd( Part *part, Relation *rel, const QString& name )
        : NamedCommand( part, name ),
        m_rel( rel )
{

    m_taken = false;
    Node *p = rel->parent() ->projectNode();
    if ( p ) {
        foreach ( Schedule * s, p->schedules() ) {
            addSchScheduled( s );
        }
    }
}
DeleteRelationCmd::~DeleteRelationCmd()
{
    if ( m_taken )
        delete m_rel;
}
void DeleteRelationCmd::execute()
{
    //kDebug()<<k_funcinfo<<m_rel->parent()<<" to "<<m_rel->child()<<endl;
    m_taken = true;
    m_rel->parent() ->takeDependChildNode( m_rel );
    m_rel->child() ->takeDependParentNode( m_rel );
    setSchScheduled( false );
    setCommandType( 1 );
}
void DeleteRelationCmd::unexecute()
{
    m_taken = false;
    m_rel->parent() ->addDependChildNode( m_rel );
    m_rel->child() ->addDependParentNode( m_rel );
    setSchScheduled();
    setCommandType( 1 );
}

ModifyRelationTypeCmd::ModifyRelationTypeCmd( Part *part, Relation *rel, Relation::Type type, const QString& name )
        : NamedCommand( part, name ),
        m_rel( rel ),
        m_newtype( type )
{

    m_oldtype = rel->type();
    Node *p = rel->parent() ->projectNode();
    if ( p ) {
        foreach ( Schedule * s, p->schedules() ) {
            addSchScheduled( s );
        }
    }
}
void ModifyRelationTypeCmd::execute()
{
    m_rel->setType( m_newtype );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyRelationTypeCmd::unexecute()
{
    m_rel->setType( m_oldtype );
    setSchScheduled();
    setCommandType( 1 );
}

ModifyRelationLagCmd::ModifyRelationLagCmd( Part *part, Relation *rel, Duration lag, const QString& name )
        : NamedCommand( part, name ),
        m_rel( rel ),
        m_newlag( lag )
{

    m_oldlag = rel->lag();
    Node *p = rel->parent() ->projectNode();
    if ( p ) {
        foreach ( Schedule * s, p->schedules() ) {
            addSchScheduled( s );
        }
    }
}
void ModifyRelationLagCmd::execute()
{
    m_rel->setLag( m_newlag );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyRelationLagCmd::unexecute()
{
    m_rel->setLag( m_oldlag );
    setSchScheduled();
    setCommandType( 1 );
}

AddResourceRequestCmd::AddResourceRequestCmd( Part *part, ResourceGroupRequest *group, ResourceRequest *request, const QString& name )
        : NamedCommand( part, name ),
        m_group( group ),
        m_request( request )
{

    m_mine = true;
}
AddResourceRequestCmd::~AddResourceRequestCmd()
{
    if ( m_mine )
        delete m_request;
}
void AddResourceRequestCmd::execute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_group<<" req="<<m_request<<endl;
    m_group->addResourceRequest( m_request );
    m_mine = false;
    setSchScheduled( false );
    setCommandType( 1 );
}
void AddResourceRequestCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_group<<" req="<<m_request<<endl;
    m_group->takeResourceRequest( m_request );
    m_mine = true;
    setSchScheduled();
    setCommandType( 1 );
}

RemoveResourceRequestCmd::RemoveResourceRequestCmd( Part *part, ResourceGroupRequest *group, ResourceRequest *request, const QString& name )
        : NamedCommand( part, name ),
        m_group( group ),
        m_request( request )
{

    m_mine = false;
    //kDebug()<<k_funcinfo<<"group req="<<group<<" req="<<request<<" to gr="<<m_group->group()<<endl;
    Task *t = request->task();
    if ( t ) { // safety, something is seriously wrong!
        foreach ( Schedule * s, t->schedules() ) {
            addSchScheduled( s );
        }
    }
}
RemoveResourceRequestCmd::~RemoveResourceRequestCmd()
{
    if ( m_mine )
        delete m_request;
}
void RemoveResourceRequestCmd::execute()
{
    m_group->takeResourceRequest( m_request );
    m_mine = true;
    setSchScheduled( false );
    setCommandType( 1 );
}
void RemoveResourceRequestCmd::unexecute()
{
    m_group->addResourceRequest( m_request );
    m_mine = false;
    setSchScheduled();
    setCommandType( 1 );
}

ModifyEffortCmd::ModifyEffortCmd( Part *part, Node &node, Duration oldvalue, Duration newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_effort( node.effort() ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void ModifyEffortCmd::execute()
{
    m_effort->set
    ( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyEffortCmd::unexecute()
{
    m_effort->set
    ( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

EffortModifyOptimisticRatioCmd::EffortModifyOptimisticRatioCmd( Part *part, Node &node, int oldvalue, int newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_effort( node.effort() ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void EffortModifyOptimisticRatioCmd::execute()
{
    m_effort->setOptimisticRatio( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void EffortModifyOptimisticRatioCmd::unexecute()
{
    m_effort->setOptimisticRatio( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

EffortModifyPessimisticRatioCmd::EffortModifyPessimisticRatioCmd( Part *part, Node &node, int oldvalue, int newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_effort( node.effort() ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void EffortModifyPessimisticRatioCmd::execute()
{
    m_effort->setPessimisticRatio( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void EffortModifyPessimisticRatioCmd::unexecute()
{
    m_effort->setPessimisticRatio( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

ModifyEffortTypeCmd::ModifyEffortTypeCmd( Part *part, Node &node, int oldvalue, int newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_effort( node.effort() ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void ModifyEffortTypeCmd::execute()
{
    m_effort->setType( static_cast<Effort::Type>( m_newvalue ) );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyEffortTypeCmd::unexecute()
{
    m_effort->setType( static_cast<Effort::Type>( m_oldvalue ) );
    setSchScheduled();
    setCommandType( 1 );
}

EffortModifyRiskCmd::EffortModifyRiskCmd( Part *part, Node &node, int oldvalue, int newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_effort( node.effort() ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void EffortModifyRiskCmd::execute()
{
    m_effort->setRisktype( static_cast<Effort::Risktype>( m_newvalue ) );
    setSchScheduled( false );
    setCommandType( 1 );
}
void EffortModifyRiskCmd::unexecute()
{
    m_effort->setRisktype( static_cast<Effort::Risktype>( m_oldvalue ) );
    setSchScheduled();
    setCommandType( 1 );
}

AddResourceGroupRequestCmd::AddResourceGroupRequestCmd( Part *part, Task &task, ResourceGroupRequest *request, const QString& name )
        : NamedCommand( part, name ),
        m_task( task ),
        m_request( request )
{

    m_mine = true;
}
void AddResourceGroupRequestCmd::execute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_request<<endl;
    m_task.addRequest( m_request );
    m_mine = false;

    setCommandType( 1 );
}
void AddResourceGroupRequestCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_request<<endl;
    m_task.takeRequest( m_request ); // group should now be empty of resourceRequests
    m_mine = true;

    setCommandType( 1 );
}

RemoveResourceGroupRequestCmd::RemoveResourceGroupRequestCmd( Part *part, ResourceGroupRequest *request, const QString& name )
        : NamedCommand( part, name ),
        m_task( request->parent() ->task() ),
        m_request( request )
{

    m_mine = false;
}

RemoveResourceGroupRequestCmd::RemoveResourceGroupRequestCmd( Part *part, Task &task, ResourceGroupRequest *request, const QString& name )
        : NamedCommand( part, name ),
        m_task( task ),
        m_request( request )
{

    m_mine = false;
}
void RemoveResourceGroupRequestCmd::execute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_request<<endl;
    m_task.takeRequest( m_request ); // group should now be empty of resourceRequests
    m_mine = true;

    setCommandType( 1 );
}
void RemoveResourceGroupRequestCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<"group="<<m_request<<endl;
    m_task.addRequest( m_request );
    m_mine = false;

    setCommandType( 1 );
}

AddResourceCmd::AddResourceCmd( Part *part, ResourceGroup *group, Resource *resource, const QString& name )
        : NamedCommand( part, name ),
        m_group( group ),
        m_resource( resource )
{
    m_index = group->indexOf( resource );
    m_mine = true;
}
AddResourceCmd::~AddResourceCmd()
{
    if ( m_mine ) {
        //kDebug()<<k_funcinfo<<"delete: "<<m_resource<<endl;
        delete m_resource;
    }
}
void AddResourceCmd::execute()
{
    Q_ASSERT( m_group->project() );
    if ( m_group->project() ) {
        m_group->project()->addResource( m_group, m_resource, m_index );
        m_mine = false;
        //kDebug()<<k_funcinfo<<"added: "<<m_resource<<endl;
    }
    setCommandType( 0 );
}
void AddResourceCmd::unexecute()
{
    Q_ASSERT( m_group->project() );
    if ( m_group->project() ) {
        m_group->project()->takeResource( m_group, m_resource );
        //kDebug()<<k_funcinfo<<"removed: "<<m_resource<<endl;
        m_mine = true;
    }
    setCommandType( 0 );
    Q_ASSERT( m_group->project() );
}

RemoveResourceCmd::RemoveResourceCmd( Part *part, ResourceGroup *group, Resource *resource, const QString& name )
        : AddResourceCmd( part, group, resource, name )
{
    //kDebug()<<k_funcinfo<<resource<<endl;
    m_mine = false;
    m_requests = m_resource->requests();

    foreach ( Schedule * s, resource->schedules() ) {
        addSchScheduled( s );
    }
}
RemoveResourceCmd::~RemoveResourceCmd()
{
    while ( !m_appointments.isEmpty() )
        delete m_appointments.takeFirst();
}
void RemoveResourceCmd::execute()
{
    foreach ( ResourceRequest * r, m_requests ) {
        r->parent() ->takeResourceRequest( r );
        //kDebug()<<"Remove request for"<<r->resource()->name()<<endl;
    }
    foreach ( Appointment * a, m_resource->appointments() ) {
        m_appointments.append( a );
    }
    foreach ( Appointment * a, m_appointments ) {
        a->detach(); //NOTE: removes from m_resource->appointments()
        //kDebug()<<k_funcinfo<<"detached: "<<a<<endl;
    }
    AddResourceCmd::unexecute();
    setSchScheduled( false );
}
void RemoveResourceCmd::unexecute()
{
    while ( !m_appointments.isEmpty() ) {
        //kDebug()<<k_funcinfo<<"attach: "<<m_appointments.first()<<endl;
        m_appointments.takeFirst() ->attach();
    }
    foreach ( ResourceRequest * r, m_requests ) {
        r->parent() ->addResourceRequest( r );
        //kDebug()<<"Add request for "<<r->resource()->name()<<endl;
    }
    AddResourceCmd::execute();
    setSchScheduled();
}

ModifyResourceNameCmd::ModifyResourceNameCmd( Part *part, Resource *resource, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->name();
}
void ModifyResourceNameCmd::execute()
{
    m_resource->setName( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceNameCmd::unexecute()
{
    m_resource->setName( m_oldvalue );

    setCommandType( 0 );
}
ModifyResourceInitialsCmd::ModifyResourceInitialsCmd( Part *part, Resource *resource, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->initials();
}
void ModifyResourceInitialsCmd::execute()
{
    m_resource->setInitials( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceInitialsCmd::unexecute()
{
    m_resource->setInitials( m_oldvalue );

    setCommandType( 0 );
}
ModifyResourceEmailCmd::ModifyResourceEmailCmd( Part *part, Resource *resource, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->email();
}
void ModifyResourceEmailCmd::execute()
{
    m_resource->setEmail( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceEmailCmd::unexecute()
{
    m_resource->setEmail( m_oldvalue );

    setCommandType( 0 );
}
ModifyResourceTypeCmd::ModifyResourceTypeCmd( Part *part, Resource *resource, int value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->type();

    foreach ( Schedule * s, resource->schedules() ) {
        addSchScheduled( s );
    }
}
void ModifyResourceTypeCmd::execute()
{
    m_resource->setType( ( Resource::Type ) m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyResourceTypeCmd::unexecute()
{
    m_resource->setType( ( Resource::Type ) m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}
ModifyResourceUnitsCmd::ModifyResourceUnitsCmd( Part *part, Resource *resource, int value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->units();

    foreach ( Schedule * s, resource->schedules() ) {
        addSchScheduled( s );
    }
}
void ModifyResourceUnitsCmd::execute()
{
    m_resource->setUnits( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyResourceUnitsCmd::unexecute()
{
    m_resource->setUnits( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

ModifyResourceAvailableFromCmd::ModifyResourceAvailableFromCmd( Part *part, Resource *resource, const DateTime& value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->availableFrom();

    if ( resource->project() ) {
        QDateTime s;
        QDateTime e;
        foreach ( Schedule * rs, resource->schedules() ) {
            Schedule * sch = resource->project() ->findSchedule( rs->id() );
            if ( sch ) {
                s = sch->start();
                e = sch->end();
                kDebug() << k_funcinfo << "old=" << m_oldvalue << " new=" << value << " s=" << s << " e=" << e << endl;
            }
            if ( !s.isValid() || !e.isValid() || ( ( m_oldvalue > s || value > s ) && ( m_oldvalue < e || value < e ) ) ) {
                addSchScheduled( rs );
            }
        }
    }
}
void ModifyResourceAvailableFromCmd::execute()
{
    m_resource->setAvailableFrom( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 ); //FIXME
}
void ModifyResourceAvailableFromCmd::unexecute()
{
    m_resource->setAvailableFrom( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 ); //FIXME
}

ModifyResourceAvailableUntilCmd::ModifyResourceAvailableUntilCmd( Part *part, Resource *resource, const DateTime& value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->availableUntil();

    if ( resource->project() ) {
        QDateTime s;
        QDateTime e;
        foreach ( Schedule * rs, resource->schedules() ) {
            Schedule * sch = resource->project() ->findSchedule( rs->id() );
            if ( sch ) {
                s = sch->start();
                e = sch->end();
                kDebug() << k_funcinfo << "old=" << m_oldvalue << " new=" << value << " s=" << s << " e=" << e << endl;
            }
            if ( !s.isValid() || !e.isValid() || ( ( m_oldvalue > s || value > s ) && ( m_oldvalue < e || value < e ) ) ) {
                addSchScheduled( rs );
            }
        }
    }
}
void ModifyResourceAvailableUntilCmd::execute()
{
    m_resource->setAvailableUntil( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 ); //FIXME
}
void ModifyResourceAvailableUntilCmd::unexecute()
{
    m_resource->setAvailableUntil( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 ); //FIXME
}

ModifyResourceNormalRateCmd::ModifyResourceNormalRateCmd( Part *part, Resource *resource, double value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->normalRate();
}
void ModifyResourceNormalRateCmd::execute()
{
    m_resource->setNormalRate( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceNormalRateCmd::unexecute()
{
    m_resource->setNormalRate( m_oldvalue );

    setCommandType( 0 );
}
ModifyResourceOvertimeRateCmd::ModifyResourceOvertimeRateCmd( Part *part, Resource *resource, double value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->overtimeRate();
}
void ModifyResourceOvertimeRateCmd::execute()
{
    m_resource->setOvertimeRate( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceOvertimeRateCmd::unexecute()
{
    m_resource->setOvertimeRate( m_oldvalue );

    setCommandType( 0 );
}

ModifyResourceCalendarCmd::ModifyResourceCalendarCmd( Part *part, Resource *resource, Calendar *value, const QString& name )
        : NamedCommand( part, name ),
        m_resource( resource ),
        m_newvalue( value )
{
    m_oldvalue = resource->calendar( true );

    foreach ( Schedule * s, resource->schedules() ) {
        addSchScheduled( s );
    }
}
void ModifyResourceCalendarCmd::execute()
{
    m_resource->setCalendar( m_newvalue );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ModifyResourceCalendarCmd::unexecute()
{
    m_resource->setCalendar( m_oldvalue );
    setSchScheduled();
    setCommandType( 1 );
}

RemoveResourceGroupCmd::RemoveResourceGroupCmd( Part *part, Project *project, ResourceGroup *group, const QString& name )
        : NamedCommand( part, name ),
        m_group( group ),
        m_project( project ),
        m_cmd( 0 )
{
    m_index = project->indexOf( group );
    m_mine = false;
    if ( !m_group->requests().isEmpty() ) {
        m_cmd = new KMacroCommand("");
        foreach( ResourceGroupRequest * r, m_group->requests() ) {
            m_cmd->addCommand( new RemoveResourceGroupRequestCmd( part, r ) );
        }
    }
}
RemoveResourceGroupCmd::~RemoveResourceGroupCmd()
{
    delete m_cmd;
    if ( m_mine )
        delete m_group;
}
void RemoveResourceGroupCmd::execute()
{
    // remove all requests to this group
    int c = 0;
    if ( m_cmd ) {
        m_cmd->execute();
        c = 1;
    }
    if ( m_project )
        m_project->takeResourceGroup( m_group );
    m_mine = true;

    setCommandType( c );
}
void RemoveResourceGroupCmd::unexecute()
{
    int c = 0;
    if ( m_project )
        m_project->addResourceGroup( m_group, m_index );

    m_mine = false;
    // add all requests
    if ( m_cmd ) {
        m_cmd->unexecute();
        c = 1;
    }
    setCommandType( c );
}

AddResourceGroupCmd::AddResourceGroupCmd( Part *part, Project *project, ResourceGroup *group, const QString& name )
        : RemoveResourceGroupCmd( part, project, group, name )
{
    m_mine = true;
}
void AddResourceGroupCmd::execute()
{
    RemoveResourceGroupCmd::unexecute();
}
void AddResourceGroupCmd::unexecute()
{
    RemoveResourceGroupCmd::execute();
}

ModifyResourceGroupNameCmd::ModifyResourceGroupNameCmd( Part *part, ResourceGroup *group, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_group( group ),
        m_newvalue( value )
{
    m_oldvalue = group->name();
}
void ModifyResourceGroupNameCmd::execute()
{
    m_group->setName( m_newvalue );

    setCommandType( 0 );
}
void ModifyResourceGroupNameCmd::unexecute()
{
    m_group->setName( m_oldvalue );

    setCommandType( 0 );
}

ModifyResourceGroupTypeCmd::ModifyResourceGroupTypeCmd( Part *part, ResourceGroup *group, int value, const QString& name )
    : NamedCommand( part, name ),
        m_group( group ),
        m_newvalue( value )
{
    m_oldvalue = group->type();
}
void ModifyResourceGroupTypeCmd::execute()
{
    m_group->setType( static_cast<ResourceGroup::Type>( m_newvalue) );

    setCommandType( 0 );
}
void ModifyResourceGroupTypeCmd::unexecute()
{
    m_group->setType( static_cast<ResourceGroup::Type>( m_oldvalue ) );

    setCommandType( 0 );
}

ModifyCompletionStartedCmd::ModifyCompletionStartedCmd( Part *part, Completion &completion, bool value, const QString& name )
        : NamedCommand( part, name ),
        m_completion( completion ),
        oldvalue( m_completion.started() ),
        newvalue( value )
{
}
void ModifyCompletionStartedCmd::execute()
{
    m_completion.setStarted( newvalue );

    setCommandType( 0 );
}
void ModifyCompletionStartedCmd::unexecute()
{
    m_completion.setStarted( oldvalue );

    setCommandType( 0 );
}

ModifyCompletionFinishedCmd::ModifyCompletionFinishedCmd( Part *part, Completion &completion, bool value, const QString& name )
        : NamedCommand( part, name ),
        m_completion( completion ),
        oldvalue( m_completion.finished() ),
        newvalue( value )
{
}
void ModifyCompletionFinishedCmd::execute()
{
    m_completion.setFinished( newvalue );

    setCommandType( 0 );
}
void ModifyCompletionFinishedCmd::unexecute()
{
    m_completion.setFinished( oldvalue );

    setCommandType( 0 );
}

ModifyCompletionStartTimeCmd::ModifyCompletionStartTimeCmd( Part *part, Completion &completion, QDateTime value, const QString& name )
        : NamedCommand( part, name ),
        m_completion( completion ),
        oldvalue( m_completion.startTime() ),
        newvalue( value )
{
}
void ModifyCompletionStartTimeCmd::execute()
{
    m_completion.setStartTime( newvalue );

    setCommandType( 0 );
}
void ModifyCompletionStartTimeCmd::unexecute()
{
    m_completion.setStartTime( oldvalue );

    setCommandType( 0 );
}

ModifyCompletionFinishTimeCmd::ModifyCompletionFinishTimeCmd( Part *part, Completion &completion, QDateTime value, const QString& name )
        : NamedCommand( part, name ),
        m_completion( completion ),
        oldvalue( m_completion.finishTime() ),
        newvalue( value )
{
}
void ModifyCompletionFinishTimeCmd::execute()
{
    m_completion.setFinishTime( newvalue );

    setCommandType( 0 );
}
void ModifyCompletionFinishTimeCmd::unexecute()
{
    m_completion.setFinishTime( oldvalue );

    setCommandType( 0 );
}

AddCompletionEntryCmd::AddCompletionEntryCmd( Part *part, Completion &completion, const QDate &date, Completion::Entry *value, const QString& name )
        : NamedCommand( part, name ),
        m_completion( completion ),
        m_date( date ),
        newvalue( value )
{
    oldvalue = m_completion.entry( date );
}
void AddCompletionEntryCmd::execute()
{
    if ( oldvalue ) {
        m_completion.takeEntry( m_date );
    }
    m_completion.addEntry( m_date, newvalue );

    setCommandType( 0 );
}
void AddCompletionEntryCmd::unexecute()
{
    m_completion.takeEntry( m_date );
    if ( oldvalue ) {
        m_completion.addEntry( m_date, oldvalue );
    }
    setCommandType( 0 );
}


AddAccountCmd::AddAccountCmd( Part *part, Project &project, Account *account, const QString& parent, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_account( account ),
        m_parent( 0 ),
        m_parentName( parent )
{
    m_mine = true;
}

AddAccountCmd::AddAccountCmd( Part *part, Project &project, Account *account, Account *parent, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_account( account ),
        m_parent( parent )
{
    m_mine = true;
}

AddAccountCmd::~AddAccountCmd()
{
    if ( m_mine )
        delete m_account;
}

void AddAccountCmd::execute()
{
    if ( m_parent == 0 && !m_parentName.isEmpty() ) {
        m_parent = m_project.accounts().findAccount( m_parentName );
    }
    m_project.accounts().insert( m_account, m_parent );

    setCommandType( 0 );
    m_mine = false;
}
void AddAccountCmd::unexecute()
{
    m_project.accounts().take( m_account );

    setCommandType( 0 );
    m_mine = true;
}

RemoveAccountCmd::RemoveAccountCmd( Part *part, Project &project, Account *account, const QString& name )
        : NamedCommand( part, name ),
        m_project( project ),
        m_account( account ),
        m_parent( account->parent() )
{
    if ( m_parent ) {
        m_index = m_parent->accountList().indexOf( account );
    } else {
        m_index = project.accounts().accountList().indexOf( account );
    }
    m_mine = false;
    m_isDefault = account == project.accounts().defaultAccount();
}

RemoveAccountCmd::~RemoveAccountCmd()
{
    if ( m_mine )
        delete m_account;
}

void RemoveAccountCmd::execute()
{
    if ( m_isDefault ) {
        m_project.accounts().setDefaultAccount( 0 );
    }
    m_project.accounts().take( m_account );

    setCommandType( 0 );
    m_mine = true;
}
void RemoveAccountCmd::unexecute()
{
    m_project.accounts().insert( m_account, m_parent, m_index );
    if ( m_isDefault ) {
        m_project.accounts().setDefaultAccount( m_account );
    }
    setCommandType( 0 );
    m_mine = false;
}

RenameAccountCmd::RenameAccountCmd( Part *part, Account *account, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_account( account )
{
    m_oldvalue = account->name();
    m_newvalue = value;
}

void RenameAccountCmd::execute()
{
    m_account->setName( m_newvalue );
    setCommandType( 0 );
}
void RenameAccountCmd::unexecute()
{
    m_account->setName( m_oldvalue );
    setCommandType( 0 );
}

ModifyAccountDescriptionCmd::ModifyAccountDescriptionCmd( Part *part, Account *account, const QString& value, const QString& name )
        : NamedCommand( part, name ),
        m_account( account )
{
    m_oldvalue = account->description();
    m_newvalue = value;
}

void ModifyAccountDescriptionCmd::execute()
{
    m_account->setDescription( m_newvalue );
    setCommandType( 0 );
}
void ModifyAccountDescriptionCmd::unexecute()
{
    m_account->setDescription( m_oldvalue );
    setCommandType( 0 );
}


NodeModifyStartupCostCmd::NodeModifyStartupCostCmd( Part *part, Node &node, double value, const QString& name )
        : NamedCommand( part, name ),
        m_node( node )
{
    m_oldvalue = node.startupCost();
    m_newvalue = value;
}

void NodeModifyStartupCostCmd::execute()
{
    m_node.setStartupCost( m_newvalue );
    setCommandType( 0 );
}
void NodeModifyStartupCostCmd::unexecute()
{
    m_node.setStartupCost( m_oldvalue );
    setCommandType( 0 );
}

NodeModifyShutdownCostCmd::NodeModifyShutdownCostCmd( Part *part, Node &node, double value, const QString& name )
        : NamedCommand( part, name ),
        m_node( node )
{
    m_oldvalue = node.startupCost();
    m_newvalue = value;
}

void NodeModifyShutdownCostCmd::execute()
{
    m_node.setShutdownCost( m_newvalue );
    setCommandType( 0 );
}
void NodeModifyShutdownCostCmd::unexecute()
{
    m_node.setShutdownCost( m_oldvalue );
    setCommandType( 0 );
}

NodeModifyRunningAccountCmd::NodeModifyRunningAccountCmd( Part *part, Node &node, Account *oldvalue, Account *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_node( node )
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<endl;
}
void NodeModifyRunningAccountCmd::execute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_oldvalue ) {
        m_oldvalue->removeRunning( m_node );
    }
    if ( m_newvalue ) {
        m_newvalue->addRunning( m_node );
    }
    setCommandType( 0 );
}
void NodeModifyRunningAccountCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_newvalue ) {
        m_newvalue->removeRunning( m_node );
    }
    if ( m_oldvalue ) {
        m_oldvalue->addRunning( m_node );
    }
    setCommandType( 0 );
}

NodeModifyStartupAccountCmd::NodeModifyStartupAccountCmd( Part *part, Node &node, Account *oldvalue, Account *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_node( node )
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<endl;
}

void NodeModifyStartupAccountCmd::execute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_oldvalue ) {
        m_oldvalue->removeStartup( m_node );
    }
    if ( m_newvalue ) {
        m_newvalue->addStartup( m_node );
    }
    setCommandType( 0 );
}
void NodeModifyStartupAccountCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_newvalue ) {
        m_newvalue->removeStartup( m_node );
    }
    if ( m_oldvalue ) {
        m_oldvalue->addStartup( m_node );
    }
    setCommandType( 0 );
}

NodeModifyShutdownAccountCmd::NodeModifyShutdownAccountCmd( Part *part, Node &node, Account *oldvalue, Account *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_node( node )
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<endl;
}

void NodeModifyShutdownAccountCmd::execute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_oldvalue ) {
        m_oldvalue->removeShutdown( m_node );
    }
    if ( m_newvalue ) {
        m_newvalue->addShutdown( m_node );
    }
    setCommandType( 0 );
}
void NodeModifyShutdownAccountCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<endl;
    if ( m_newvalue ) {
        m_newvalue->removeShutdown( m_node );
    }
    if ( m_oldvalue ) {
        m_oldvalue->addShutdown( m_node );
    }
    setCommandType( 0 );
}

ModifyDefaultAccountCmd::ModifyDefaultAccountCmd( Part *part, Accounts &acc, Account *oldvalue, Account *newvalue, const QString& name )
        : NamedCommand( part, name ),
        m_accounts( acc )
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //kDebug()<<k_funcinfo<<endl;
}

void ModifyDefaultAccountCmd::execute()
{
    //kDebug()<<k_funcinfo<<endl;
    m_accounts.setDefaultAccount( m_newvalue );
    setCommandType( 0 );
}
void ModifyDefaultAccountCmd::unexecute()
{
    //kDebug()<<k_funcinfo<<endl;
    m_accounts.setDefaultAccount( m_oldvalue );
    setCommandType( 0 );
}

ProjectModifyConstraintCmd::ProjectModifyConstraintCmd( Part *part, Project &node, Node::ConstraintType c, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newConstraint( c ),
        oldConstraint( static_cast<Node::ConstraintType>( node.constraint() ) )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void ProjectModifyConstraintCmd::execute()
{
    m_node.setConstraint( newConstraint );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ProjectModifyConstraintCmd::unexecute()
{
    m_node.setConstraint( oldConstraint );
    setSchScheduled();
    setCommandType( 1 );
}

ProjectModifyStartTimeCmd::ProjectModifyStartTimeCmd( Part *part, Project &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.startTime() )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}

void ProjectModifyStartTimeCmd::execute()
{
    m_node.setConstraintStartTime( newTime );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ProjectModifyStartTimeCmd::unexecute()
{
    m_node.setConstraintStartTime( oldTime );
    setSchScheduled();
    setCommandType( 1 );
}

ProjectModifyEndTimeCmd::ProjectModifyEndTimeCmd( Part *part, Project &node, const QDateTime& dt, const QString& name )
        : NamedCommand( part, name ),
        m_node( node ),
        newTime( dt ),
        oldTime( node.endTime() )
{

    foreach ( Schedule * s, node.schedules() ) {
        addSchScheduled( s );
    }
}
void ProjectModifyEndTimeCmd::execute()
{
    m_node.setEndTime( newTime );
    m_node.setConstraintEndTime( newTime );
    setSchScheduled( false );
    setCommandType( 1 );
}
void ProjectModifyEndTimeCmd::unexecute()
{
    m_node.setConstraintEndTime( oldTime );
    setSchScheduled();
    setCommandType( 1 );
}

//----------------------------
AddScheduleManagerCmd::AddScheduleManagerCmd( Part *part, Project &node, ScheduleManager *sm, const QString& name )
    : NamedCommand( part, name ),
    m_node( node ),
    m_sm( sm ),
    m_mine( true)
{
}

AddScheduleManagerCmd::~AddScheduleManagerCmd()
{
    if ( m_mine )
        delete m_sm;
}

void AddScheduleManagerCmd::execute()
{
    m_node.addScheduleManager( m_sm );
    m_sm->setDeleted( false );
    m_mine = false;
}

void AddScheduleManagerCmd::unexecute()
{
    m_node.takeScheduleManager( m_sm );
    m_sm->setDeleted( true );
    m_mine = true;
}

DeleteScheduleManagerCmd::DeleteScheduleManagerCmd( Part *part, Project &node, ScheduleManager *sm, const QString& name )
    : AddScheduleManagerCmd( part, node, sm, name )
{
}

void DeleteScheduleManagerCmd::execute()
{
    AddScheduleManagerCmd::unexecute();
}

void DeleteScheduleManagerCmd::unexecute()
{
    AddScheduleManagerCmd::execute();
}

ModifyScheduleManagerNameCmd::ModifyScheduleManagerNameCmd( Part *part, ScheduleManager &sm, const QString& value, const QString& name )
    : NamedCommand( part, name ),
    m_sm( sm ),
    oldvalue( sm.name() ),
    newvalue( value )
{
}

void ModifyScheduleManagerNameCmd::execute()
{
    m_sm.setName( newvalue );
}

void ModifyScheduleManagerNameCmd::unexecute()
{
    m_sm.setName( oldvalue );
}

ModifyScheduleManagerAllowOverbookingCmd::ModifyScheduleManagerAllowOverbookingCmd( Part *part, ScheduleManager &sm, bool value, const QString& name )
    : NamedCommand( part, name ),
    m_sm( sm ),
    oldvalue( sm.allowOverbooking() ),
    newvalue( value )
{
}

void ModifyScheduleManagerAllowOverbookingCmd::execute()
{
    m_sm.setAllowOverbooking( newvalue );
}

void ModifyScheduleManagerAllowOverbookingCmd::unexecute()
{
    m_sm.setAllowOverbooking( oldvalue );
}

ModifyScheduleManagerDistributionCmd::ModifyScheduleManagerDistributionCmd( Part *part, ScheduleManager &sm, bool value, const QString& name )
    : NamedCommand( part, name ),
    m_sm( sm ),
    oldvalue( sm.usePert() ),
    newvalue( value )
{
}

void ModifyScheduleManagerDistributionCmd::execute()
{
    m_sm.setUsePert( newvalue );
}

void ModifyScheduleManagerDistributionCmd::unexecute()
{
    m_sm.setUsePert( oldvalue );
}

ModifyScheduleManagerCalculateAllCmd::ModifyScheduleManagerCalculateAllCmd( Part *part, ScheduleManager &sm, bool value, const QString& name )
    : NamedCommand( part, name ),
    m_sm( sm ),
    oldvalue( sm.calculateAll() ),
    newvalue( value )
{
}

void ModifyScheduleManagerCalculateAllCmd::execute()
{
    m_sm.setCalculateAll( newvalue );
}

void ModifyScheduleManagerCalculateAllCmd::unexecute()
{
    m_sm.setCalculateAll( oldvalue );
}

CalculateScheduleCmd::CalculateScheduleCmd( Part *part, Project &node, ScheduleManager &sm, const QString& name )
    : NamedCommand( part, name ),
    m_node( node ),
    m_sm( sm ),
    m_first( true ),
    m_oldexpected( m_sm.expected() ),
    m_oldoptimistic( m_sm.optimistic() ),
    m_oldpessimistic( m_sm.pessimistic() ),
    m_newexpected( 0 ),
    m_newoptimistic( 0 ),
    m_newpessimistic( 0 )
{
 
}

void CalculateScheduleCmd::execute()
{
    m_sm.setDeleted( true );
    
    if ( m_first ) {
        m_first = false;
        m_node.calculate( m_sm );
        m_newexpected = m_sm.expected();
        m_newoptimistic = m_sm.optimistic();
        m_newpessimistic = m_sm.pessimistic();
        return;
    }
    m_sm.setExpected( m_newexpected );
    m_sm.setOptimistic( m_newoptimistic );
    m_sm.setPessimistic( m_newpessimistic );
    m_sm.setDeleted( false );
    
}

void CalculateScheduleCmd::unexecute()
{
    m_sm.setDeleted( true );
    
    m_sm.setExpected( m_oldexpected );
    m_sm.setOptimistic( m_oldoptimistic );
    m_sm.setPessimistic( m_oldpessimistic );
    
    m_sm.setDeleted( false );
}

//------------------------
ModifyStandardWorktimeYearCmd::ModifyStandardWorktimeYearCmd( Part *part, StandardWorktime *wt, double oldvalue, double newvalue, const QString& name )
        : NamedCommand( part, name ),
        swt( wt ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{
}
void ModifyStandardWorktimeYearCmd::execute()
{
    swt->setYear( m_newvalue );
    setCommandType( 0 );
}
void ModifyStandardWorktimeYearCmd::unexecute()
{
    swt->setYear( m_oldvalue );
    setCommandType( 0 );
}

ModifyStandardWorktimeMonthCmd::ModifyStandardWorktimeMonthCmd( Part *part, StandardWorktime *wt, double oldvalue, double newvalue, const QString& name )
        : NamedCommand( part, name ),
        swt( wt ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{
}
void ModifyStandardWorktimeMonthCmd::execute()
{
    swt->setMonth( m_newvalue );
    setCommandType( 0 );
}
void ModifyStandardWorktimeMonthCmd::unexecute()
{
    swt->setMonth( m_oldvalue );
    setCommandType( 0 );
}

ModifyStandardWorktimeWeekCmd::ModifyStandardWorktimeWeekCmd( Part *part, StandardWorktime *wt, double oldvalue, double newvalue, const QString& name )
        : NamedCommand( part, name ),
        swt( wt ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{
}
void ModifyStandardWorktimeWeekCmd::execute()
{
    swt->setWeek( m_newvalue );
    setCommandType( 0 );
}
void ModifyStandardWorktimeWeekCmd::unexecute()
{
    swt->setWeek( m_oldvalue );
    setCommandType( 0 );
}

ModifyStandardWorktimeDayCmd::ModifyStandardWorktimeDayCmd( Part *part, StandardWorktime *wt, double oldvalue, double newvalue, const QString& name )
        : NamedCommand( part, name ),
        swt( wt ),
        m_oldvalue( oldvalue ),
        m_newvalue( newvalue )
{
}

void ModifyStandardWorktimeDayCmd::execute()
{
    swt->setDay( m_newvalue );
    setCommandType( 0 );
}
void ModifyStandardWorktimeDayCmd::unexecute()
{
    swt->setDay( m_oldvalue );
    setCommandType( 0 );
}


}  //KPlato namespace
