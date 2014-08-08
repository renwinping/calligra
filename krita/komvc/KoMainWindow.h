/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000-2004 David Faure <faure@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef KOMVCWINDOW_H
#define KOMVCWINDOW_H

#include "komvc_export.h"

#include <kxmlguiwindow.h>
#include <kurl.h>
#include <KoCanvasObserverBase.h>
#include <KoCanvasSupervisor.h>

class KoMainWindowPrivate;
class KoDocument;
class KoPart;
class KoView;
class KoPrintJob;
class KoDockFactoryBase;
class KRecentFilesAction;
class QDockWidget;

struct KoPageLayout;

// Calligra class but not in main module
class KoDockerManager;

/**
 * @brief Main window for a Calligra application
 *
 * This class is used to represent a main window
 * of a Calligra component. Each main window contains
 * a menubar and some toolbars.
 *
 * @note This class does NOT need to be subclassed in your application.
 */
class KOMVC_EXPORT KoMainWindow : public KXmlGuiWindow, public KoCanvasSupervisor
{
    Q_OBJECT
public:

    /**
     *  Constructor.
     *
     *  Initializes a Calligra main window (with its basic GUI etc.).
     */
    explicit KoMainWindow(KoPart *part, const KComponentData &instance);

    /**
     *  Destructor.
     */
    virtual ~KoMainWindow();

    /**
     * Update caption from document info - call when document info
     * (title in the about page) changes.
     */
    void updateCaption();


    // If noCleanup is set, KoMainWindow will not delete the root document
    // or part manager on destruction.
    void setNoCleanup(bool noCleanup);

    /**
     * Add a the given view to the list of views of this mainwindow.
     */
    void addView(KoView *view);

    /**
     * @brief showView shows the given view. Override this if you want to show
     * the view in a different way than by making it the central widget, for instance
     * as an QMdiSubWindow
     */
    virtual void showView(KoView *view);

    /**
     * @returns the currently active view
     */
    KoView *activeView() const;

    KoPart* part();


private:

    /**
     * The application should call this to show or hide a toolbar.
     * It also takes care of the corresponding action in the settings menu.
     */
    void showToolbar(const char * tbName, bool shown);

    /**
     * @return TRUE if the toolbar @p tbName is visible
     */
    bool toolbarIsVisible(const char *tbName);

public:

    /**
     * Sets the maximum number of recent documents entries.
     */
    void setMaxRecentItems(uint _number);

    /**
     * The document opened a URL -> store into recent documents list.
     */
    void addRecentURL(const KUrl& url);

    /**
     * Load the desired document and show it.
     * @param url the URL to open
     *
     * @return TRUE on success.
     */
    bool openDocument(const KUrl & url);

private:
    friend class KoApplication;
    /**
     * Create a document, open the given url, create a view, set the document
     * on the view and add the view to the mainwindow.
     *
     * Special method for KoApplication::start, don't use.
     */
    KoDocument *createDocumentFromUrl(const KUrl & url);

private:

    /**
     * Reloads the recent documents list.
     */
    void reloadRecentFileList();

    /**
     * Updates the window caption based on the document info and path.
     */
    void updateCaption(const QString & caption, bool mod);
    void updateReloadFileAction(KoDocument *doc);

public:
    void setReadWrite(bool readwrite);

    /**
     * Returns the dockwidget specified by the @p factory. If the dock widget doesn't exist yet it's created.
     * Add a "view_palette_action_menu" action to your view menu if you want to use closable dock widgets.
     * @param factory the factory used to create the dock widget if needed
     * @return the dock widget specified by @p factory (may be 0)
     */
    QDockWidget* createDockWidget(KoDockFactoryBase* factory);

    /// Return the list of dock widgets belonging to this main window.
    QList<QDockWidget*> dockWidgets();

    QList<KoCanvasObserverBase*> canvasObservers();

    /**
     * @return the KoDockerManager which is assigned
     * WARNING: this could be 0, if no docker have been assigned yet. In that case create one
      * and assign it.
     * @ref setDockerManager to assign it.
     */
    KoDockerManager * dockerManager() const;

signals:

    /**
     * This signal is emitted if the document has been saved successfully.
     */
    void documentSaved();

    /// This signal is emitted when this windows has finished loading of a
    /// document. The document may be opened in another window in the end.
    /// In this case, the signal means there is no link between the window
    /// and the document anymore.
    void loadCompleted();

    /// This signal is emitted right after the docker states have been succefully restored from config
    void restoringDone();

    /// This signal is emitted when the color theme changes
    void themeChanged();

    /// This signal is emitted when the shortcut key configuration has changed
    void keyBindingsChanged();

public slots:

    /**
     * Slot for eMailing the document using KMail
     *
     * This is a very simple extension that will allow any document
     * that is currently being edited to be emailed using KMail.
     */
    void slotEmailFile();

    /**
     *  Slot for opening a new document.
     *
     *  If the current document is empty, the new document replaces it.
     *  If not, a new mainwindow will be opened for showing the document.
     */
    void slotFileNew();

    /**
     *  Slot for opening a saved file.
     *
     *  If the current document is empty, the opened document replaces it.
     *  If not a new mainwindow will be opened for showing the opened file.
     */
    void slotFileOpen();

    /**
     *  Slot for opening a file among the recently opened files.
     *
     *  If the current document is empty, the opened document replaces it.
     *  If not a new mainwindow will be opened for showing the opened file.
     */
    void slotFileOpenRecent(const KUrl &);
public slots:
    /**
     *  Saves the current document with the current name.
     */
    void slotFileSave();
private slots:
    /**
     *  Saves the current document with a new name.
     */
    void slotFileSaveAs();
public slots:
    /**
     *  Prints the actual document.
     */
    void slotFilePrint();

private slots:
    void slotFilePrintPreview();
public slots:
    KoPrintJob* exportToPdf(const QString &pdfFileName = QString());
private slots:
    KoPrintJob* exportToPdf(KoPageLayout pageLayout, QString pdfFileName = QString());

    /**
     * Show a dialog with author and document information.
     */
    void slotDocumentInfo();

    /**
     *  Closes the document.
     */
    void slotFileClose();


    /**
     * Closes all open documents.
     */
    void slotFileCloseAll();

    /**
     *  Closes the mainwindow.
     */
    void slotFileQuit();

    /**
     *  Configure key bindings.
     */
    void slotConfigureKeys();

    /**
     *  Configure toolbars.
     */
    void slotConfigureToolbars();

    /**
     *  Post toolbar config.
     * (Plug action lists back in, etc.)
     */
    void slotNewToolbarConfig();

    /**
     *  Shows or hides a toolbar
     */
    void slotToolbarToggled(bool toggle);

    /**
     * Toggle full screen on/off.
     */
    void viewFullscreen(bool fullScreen);

    /**
     * Reload file
     */
    void slotReloadFile();

    /**
     * File --> Import
     *
     * This will call slotFileOpen().  To differentiate this from an ordinary
     * call to slotFileOpen() call @ref isImporting().
     */
    void slotImportFile();

    /**
     * File --> Export
     *
     * This will call slotFileSaveAs().  To differentiate this from an ordinary
     * call to slotFileSaveAs() call @ref isExporting().
     */
    void slotExportFile();

    void slotEncryptDocument();
    void slotUncompressToDir();
public slots:
    void slotProgress(int value);
private slots:
    /**
     * Hide the dockers
     */
    void toggleDockersVisibility(bool visible);
public slots:
    /**
     * Saves the document, asking for a filename if necessary.
     *
     * @param saveas if set to TRUE the user is always prompted for a filename
     *
     * @param silent if set to TRUE rootDocument()->setTitleModified will not be called.
     *
     * @param specialOutputFlag set to enums defined in KoDocument if save to special output format
     *
     * @return TRUE on success, false on error or cancel
     *         (don't display anything in this case, the error dialog box is also implemented here
     *         but restore the original URL in slotFileSaveAs)
     */
    bool saveDocument(KoDocument *document, bool saveas = false, bool silent = false, int specialOutputFlag = 0);
private slots:
    void undo();
    void redo();
private:

    /**
     * This setting indicates who is calling chooseNewDocument.
     * Usually the app will want to
     * - show the template dialog with 'everything' if InitDocAppStarting, InitDocFileClose or InitDocEmbedded
     * - show the template dialog with 'templates only' if InitDocFileNew
     * - create an empty document with default settings if InitDocEmpty
     */
    enum InitDocFlags { /*InitDocAppStarting, */ InitDocFileNew, InitDocFileClose /*, InitDocEmbedded, InitDocEmpty*/ };
protected:
    void closeEvent(QCloseEvent * e);
    void resizeEvent(QResizeEvent * e);
private:
    /**
     * Ask user about saving changes to the document upon exit.
     */
    bool queryClose();

    bool openDocumentInternal(const KUrl &url, KoDocument *newdoc = 0);

    /**
     * Returns whether or not the current slotFileSave[As]() or saveDocument()
     * call is actually an export operation (like File --> Export).
     *
     * If this is true, you must call KoDocument::export() instead of
     * KoDocument::save() or KoDocument::saveAs(), in any reimplementation of
     * saveDocument().
     */
    bool isExporting() const;

    /**
     * Returns whether or not the current slotFileOpen() or openDocument()
     * call is actually an import operation (like File --> Import).
     *
     * If this is true, you must call KoDocument::import() instead of
     * KoDocument::openUrl(), in any reimplementation of openDocument() or
     * openDocumentInternal().
     */
    bool isImporting() const;

    KRecentFilesAction *recentAction() const;

private slots:
    /**
     * Save the list of recent files.
     */
    void saveRecentFiles();

    void slotLoadCompleted();
    void slotLoadCanceled(const QString &);
    void slotSaveCompleted();
    void slotSaveCanceled(const QString &);
    void forceDockTabFonts();

// ---------------------  PartManager
protected:

    /**
     * Sets the active View.
     */
    void createMainwindowGUI();
    void setToolbarList(QList<QAction*> toolbarList);
private slots:

    /**
     * @internal
     */
    void slotDocumentTitleModified(const QString &caption, bool mod);

// ---------------------  PartManager

private:

    /**
     * Asks the user if they really want to save the document.
     * Called only if outputFormat != nativeFormat.
     *
     * @return true if the document should be saved
     */
    bool exportConfirmation(const QByteArray &outputFormat);

    void saveWindowSettings();

private:

    KoMainWindowPrivate * const d;
};

#endif
