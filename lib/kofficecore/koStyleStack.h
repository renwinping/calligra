/* This file is part of the KDE project
   Copyright (c) 2003 Lukas Tinkl <lukas@kde.org>
   Copyright (c) 2003 David Faure <faure@kde.org>

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

#ifndef KOSTYLESTACK_H
#define KOSTYLESTACK_H

#include <kdemacros.h>

#include <qvaluelist.h>
#include <qdom.h>
#include <qvaluestack.h>

/**
 *  @brief This class implements a stack for the different styles of an object.
 *
 *  There can be several styles that are valid for one object. For example
 *  a textobject on a page has styles 'pr3' and 'P7' and a paragraph in
 *  that textobject has styles 'P1' and 'T3'. And some styles even have
 *  parent-styles...
 *
 *  If you want to know if there is, for example,  the attribute 'fo:font-family'
 *  for this paragraph, you have to look into style 'T3', 'P1', 'P7' and 'pr3'.
 *  When you find this attribute in one style you have to stop processing the list
 *  and take the found attribute for this object.
 *
 *  This is what this class does. You can push styles on the stack while walking
 *  through the xml-tree to your object and then ask the stack if any of the styles
 *  provides a certain attribute. The stack will search from top to bottom, i.e.
 *  in our example from 'T3' to 'pr3' and return the first occurrence of the wanted
 *  attribute.
 *
 *  So this is some sort of inheritance where the styles on top of the stack overwrite
 *  the same attribute of a lower style on the stack.
 */
class KoStyleStack
{
public:
    KoStyleStack();
    virtual ~KoStyleStack();

    /**
     * Clears the complete stack.
     */
    void clear();

    /**
     * Save the current state of the stack. Any items added between
     * this call and its corresponding restore() will be removed when calling restore().
     */
    void save();

    /**
     * Restore the stack to the state it was at the corresponding save() call.
     */
    void restore();

    /**
     * Removes the style on top of the stack.
     */
    void pop();

    /**
     * Pushes the new style onto the stack.
     */
    void push( const QDomElement& style );

    /**
     * Check if any of the styles on the stack has an attribute called 'name'-'detail'
     * where detail is e.g. left, right, top or bottom.
     * This allows to also find 'name' alone (e.g. padding implies padding-left, padding-right etc.)
     */
    bool hasAttribute( const QString& name, const QString& detail = QString::null ) const KDE_DEPRECATED;

    /**
     * Search for the attribute called 'name', starting on top of the stack,
     * and return it.
     */
    QString attribute( const QString& name, const QString& detail = QString::null ) const KDE_DEPRECATED;

    /**
     * Check if any of the styles on the stack has an attribute called 'name'-'detail'
     * where detail is e.g. left, right, top or bottom.
     * This allows to also find 'name' alone (e.g. padding implies padding-left, padding-right etc.)
     */
    bool hasAttributeNS( const char* nsURI, const char* localName, const char* detail = 0 ) const;

    /**
     * Search for the attribute called 'name', starting on top of the stack,
     * and return it.
     */
    QString attributeNS( const char* nsURI, const char* localName, const char* detail = 0 ) const;

    /**
     * Check if any of the styles on the stack has a child node called 'name'.
     */
    bool hasChildNode( const QString & name ) const KDE_DEPRECATED;

    /**
     * Search for a child node called 'name', starting on top of the stack,
     * and return it.
     */
    QDomElement childNode( const QString & name ) const KDE_DEPRECATED;

    /**
     * Check if any of the styles on the stack has a child element called 'localName' in the namespace 'nsURI'.
     */
    bool hasChildNodeNS( const char* nsURI, const char* localName ) const;

    /**
     * Search for a child element which has a child element called 'localName'
     * in the namespace 'nsURI' starting on top of the stack,
     * and return it.
     */
    QDomElement childNodeNS( const char* nsURI, const char* localName ) const;

    /**
     * Special case for the current font size, due to special handling of fo:font-size="115%".
     */
    double fontSize() const;

    /**
     * Return the name of the style specified by the user,
     * i.e. not an auto style
     */
    QString userStyleName() const;

    /**
     * Add properties name
     */
    void setTypeProperties(const char* typeProperties);

private:
    /// For save/restore: stack of "marks". Each mark is an index in m_stack.
    QValueStack<int> m_marks;

    /**
     * We use QValueList instead of QValueStack because we need access to all styles
     * not only the top one.
     */
    QValueList<QDomElement> m_stack;

    KoStyleStack( const KoStyleStack& );
    void operator=( const KoStyleStack& );
    QCString m_propertiesTagName;
};

#endif /* KOSTYLESTACK_H */
