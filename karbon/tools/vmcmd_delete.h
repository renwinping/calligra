/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
*/

#ifndef __VMCMDDELETE_H__
#define __VMCMDDELETE_H__

#include "vcommand.h"
#include <qptrlist.h>

// Delete object(s)

class VPath;

class VMCmdDelete : public VCommand
{
public:
	VMCmdDelete( KarbonPart* part );
	virtual ~VMCmdDelete() {}

	virtual void execute();
	virtual void unexecute();

private:
	QPtrList<VObject> m_objects;
};

#endif
