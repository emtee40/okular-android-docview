/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGEVIEWANNOTATOR_H_
#define _OKULAR_PAGEVIEWANNOTATOR_H_

#include <QLinkedList>
#include <QObject>
#include <qdom.h>

#include <KActionCollection>

#include "annotationtools.h"
#include "pageviewutils.h"

class QKeyEvent;
class QMouseEvent;
class QPainter;
class AnnotationActionHandler;

namespace Okular
{
class Document;
}

// engines are defined and implemented in the cpp
class AnnotatorEngine;
class AnnotationTools;
class PageView;

/**
 * @short PageView object devoted to annotation creation/handling.
 *
 * PageViewAnnotator is the okular class used for visually creating annotations.
 * It uses internal 'engines' for interacting with user events and attaches
 * the newly created annotation to the document when the creation is complete.
 * In the meanwhile all PageView events (actually mouse/paint ones) are routed
 * to this class that performs a rough visual representation of what the
 * annotation will become when finished.
 *
 * m_toolsDefinition is a AnnotationTools object that wraps a DOM object that
 * contains Annotations/Engine association for the items placed in the toolbar.
 * The XML is parsed after selecting a toolbar item, in which case an Ann is
 * initialized with the values in the XML and an engine is created to handle
 * that annotation. m_toolsDefinition is created in reparseConfig according to
 * user configuration. m_toolsDefinition is updated (and saved to disk) (1) each
 * time a property of an annotation (color, font, etc) is changed by the user,
 * and (2) each time a "quick annotation" is selected, in which case the properties
 * of the selected quick annotation are written over those of the corresponding
 * builtin tool
 */
class PageViewAnnotator : public QObject
{
    Q_OBJECT
public:
    static const int STAMP_TOOL_ID;

    PageViewAnnotator(PageView *parent, Okular::Document *storage);
    ~PageViewAnnotator() override;

    // methods used when creating the annotation
    // @return Is a tool currently selected?
    bool active() const;
    // @return Are we currently annotating (using the selected tool)?
    bool annotating() const;

    // returns the preferred cursor for the current tool. call this only
    // if active() == true
    QCursor cursor() const;

    QRect routeMouseEvent(QMouseEvent *event, PageViewItem *item);
    QRect routeTabletEvent(QTabletEvent *event, PageViewItem *item, const QPoint localOriginInGlobal);
    QRect performRouteMouseOrTabletEvent(const AnnotatorEngine::EventType eventType, const AnnotatorEngine::Button button, const QPointF pos, PageViewItem *item);
    bool routeKeyEvent(QKeyEvent *event);
    bool routePaints(const QRect wantedRect) const;
    void routePaint(QPainter *painter, const QRect paintRect);

    void reparseConfig();

    static QString defaultToolName(const QDomElement &toolElement);
    static QPixmap makeToolPixmap(const QDomElement &toolElement);

    // methods related to the annotation actions
    void setupActions(KActionCollection *ac);
    // setup those actions that first require the GUI is fully created
    void setupActionsPostGUIActivated();
    // @return Is continuous mode active (pin annotation)?
    bool continuousMode();
    // enable/disable the annotation actions
    void setToolsEnabled(bool enabled);
    // enable/disable the text-selection annotation actions
    void setTextToolsEnabled(bool enabled);

    // selects the active tool
    void selectTool(int toolID);
    // selects a stamp tool and sets the stamp symbol
    void selectStampTool(const QString &stampSymbol);
    // makes a quick annotation the active tool
    int setQuickTool(int toolID);
    // deselects the tool and uncheck all the annotation actions
    void detachAnnotation();

    // returns the builtin annotation tool with the given Id
    QDomElement builtinTool(int toolID);
    // returns the quick annotation tool with the given Id
    QDomElement quickTool(int toolID);

    // methods that write the properties
    void setAnnotationWidth(double width);
    void setAnnotationColor(const QColor &color);
    void setAnnotationInnerColor(const QColor &color);
    void setAnnotationOpacity(double opacity);
    void setAnnotationFont(const QFont &font);

public Q_SLOTS:
    void setContinuousMode(bool enabled);
    void addToQuickAnnotations();
    void slotAdvancedSettings();

Q_SIGNALS:
    void toolSelected();

private:
    // save the annotation tools to Okular settings
    void saveAnnotationTools();
    // returns the engine QDomElement of the the currently active tool
    QDomElement currentEngineElement();
    // returns the annotation QDomElement of the the currently active tool
    QDomElement currentAnnotationElement();

    // global class pointers
    Okular::Document *m_document;
    PageView *m_pageView;
    AnnotationActionHandler *m_actionHandler;
    AnnotatorEngine *m_engine;
    AnnotationTools *m_toolsDefinition;
    AnnotationTools *m_quickToolsDefinition;
    bool m_continuousMode;

    // creation related variables
    int m_lastToolID;
    QRect m_lastDrawnRect;
    PageViewItem *m_lockedItem;
    // selected annotation name
    // QString m_selectedAnnotationName;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
