/* This file is part of the KDE project
   Copyright (C) 2001, 2002, 2003 The Karbon Developers

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

#ifndef __VTEXTTOOL_H__
#define __VTEXTTOOL_H__


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kdialogbase.h>

#include "qframe.h"
#include "qgroupbox.h"
#include "qcombobox.h"

#include "vcommand.h"
#include "vtext.h"
#include "vtool.h"

class KFontCombo;
class KIntNumInput;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTabWidget;
class ShadowWidget;
class VTextTool;


class ShadowPreview : public QWidget
{
	Q_OBJECT

public:
	ShadowPreview( ShadowWidget* parent );
	~ShadowPreview();

signals:
	void changed( int angle, int distance, bool );

protected:
	virtual void mouseReleaseEvent( QMouseEvent* );
	virtual void paintEvent( QPaintEvent* );

private:
	ShadowWidget* m_parent;
};


class ShadowWidget : public QGroupBox
{
	Q_OBJECT

public:
	ShadowWidget( QWidget* parent, const char* name, int angle, int distance, bool translucent );
	~ShadowWidget();

	void setUseShadow( bool use );
	bool useShadow();
	void setShadowAngle( int angle );
	int shadowAngle();
	void setShadowDistance( int distance );
	int shadowDistance();
	void setTranslucent( bool translucent );
	bool isTranslucent();

public slots:
	void setShadowValues( int angle, int distance, bool translucent );
	void updatePreview( int );
	void updatePreview();

protected:
	QCheckBox* m_useShadow;
	KIntNumInput* m_angle;
	KIntNumInput* m_distance;
	QCheckBox* m_translucent;
	ShadowPreview* m_preview;
};


class VTextOptionsWidget : public KDialogBase
{
	Q_OBJECT

public:
	VTextOptionsWidget( VTextTool* tool, QWidget *parent );
	~VTextOptionsWidget();

	void setFont( const QFont& font );
	QFont font();
	void setText( const QString& text );
	QString text();
	void setPosition( VText::Position position );
	VText::Position position();
	void setAlignment( VText::Alignment alignment );
	VText::Alignment alignment();
	void setUseShadow( bool state );
	bool useShadow();
	void setShadow( int angle, int distance, bool translucent );
	bool translucentShadow();
	int shadowAngle();
	int shadowDistance();

public slots:
	void valueChanged( int );
	void accept();
	void textChanged( const QString& );
	void editBasePath();
	void convertToShapes();

protected:
	QTabWidget* m_tabWidget;
	KFontCombo* m_fontCombo;
	QCheckBox* m_boldCheck;
	QCheckBox* m_italicCheck;
	KIntNumInput* m_fontSize;
	QLineEdit* m_textEditor;
	ShadowWidget* m_shadow;
	QComboBox* m_textAlignment;
	QComboBox* m_textPosition;
	QPushButton* m_editBasePath;
	QPushButton* m_convertToShapes;

	VTextTool* m_tool;
};


class VTextTool : public VTool, public VVisitor
{
public:
	VTextTool( KarbonPart *part, const char* name );
	~VTextTool();

	virtual QString name() { return i18n( "Text Tool" ); }
	virtual QString icon() { return "14_text"; }

	virtual QString contextHelp();
	virtual bool showDialog() const;

	virtual void activate();
	virtual void deactivate();

	virtual void mouseButtonPress();
	virtual void mouseButtonRelease();
	virtual void mouseDrag();
	virtual void mouseDragRelease();
	virtual void textChanged();
	virtual void accept();
	virtual void cancel();
	virtual void editBasePath();
	virtual void convertToShapes();

	virtual void visitVComposite( VComposite& composite );
	virtual void visitVDocument( VDocument& )
	{}

	virtual void visitVGroup( VGroup& )
	{}

	virtual void visitVLayer( VLayer& )
	{}

	virtual void visitVPath( VPath& path );
	virtual void visitVText( VText& text );

private:
	class VTextCmd : public VCommand
	{
	public:
		VTextCmd( VDocument* doc, const QString& name, VText* text );
		VTextCmd( VDocument* doc, const QString& name, VText* text,
				  const QFont &newFont, const VPath& newBasePath, VText::Position newPosition, VText::Alignment newAlignment, const QString& newText,
				  bool newUseShadow, int newShadowAngle, int newShadowDistance, bool newTranslucentShadow );
		virtual ~VTextCmd();

		virtual void execute();
		virtual void unexecute();
		virtual bool isExecuted()
		{
			return m_executed;
		}

	private:
		class VTextModifPrivate
		{
		public:
			VTextModifPrivate() : oldBasePath( 0L ), newBasePath( 0L )
			{}

			QFont oldFont;
			QFont newFont;
			VPath oldBasePath;
			VPath newBasePath;
			VText::Position oldPosition;
			VText::Position newPosition;
			VText::Alignment oldAlignment;
			VText::Alignment newAlignment;
			QString oldText;
			QString newText;
			bool oldUseShadow;
			bool newUseShadow;
			int oldShadowAngle;
			int newShadowAngle;
			int oldShadowDistance;
			int newShadowDistance;
			bool oldTranslucentShadow;
			bool newTranslucentShadow;
		};

		VText* m_text;
		bool m_executed;
		VTextModifPrivate* m_textModifications;
	};

	class VTextToCompositeCmd : public VCommand
	{
	public:
		VTextToCompositeCmd( VDocument* doc, const QString& name, VText* text );
		virtual ~VTextToCompositeCmd();

		virtual void execute();
		virtual void unexecute();
		virtual bool isExecuted()
		{
			return m_executed;
		}

	private:
		VText* m_text;
		VGroup* m_group;
		bool m_executed;
	};

	void drawPathCreation();
	void drawEditedText();

	VTextOptionsWidget* m_optionsWidget;
	KoPoint m_last;
	VText* m_text;
	VText* m_editedText;
	bool m_creating;
};

#endif

