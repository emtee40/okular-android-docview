/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlggeneral.h"

#include <KAuthorized>

#include <config-okular.h>

#include "settings.h"
#include "ui_dlggeneralbase.h"

DlgGeneral::DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode)
    : QWidget(parent)
{
    m_dlg = new Ui_DlgGeneralBase();
    m_dlg->setupUi(this);

    if (embedMode == Okular::ViewerWidgetMode) {
        m_dlg->kcfg_SyncThumbnailsViewport->setVisible(false);
        m_dlg->titlebarControlCombobox->setVisible(false);
        m_dlg->kcfg_WatchFile->setVisible(false);
        m_dlg->kcfg_rtlReadingDirection->setVisible(false);
    }

    m_dlg->kcfg_BackgroundColor->setEnabled(Okular::Settings::useCustomBackgroundColor());
    m_dlg->kcfg_ShellOpenFileInTabs->setVisible(embedMode == Okular::NativeShellMode);
    m_dlg->kcfg_SwitchToTabIfOpen->setEnabled(Okular::Settings::shellOpenFileInTabs());

    connect(m_dlg->kcfg_UseCustomBackgroundColor, &QCheckBox::toggled, m_dlg->kcfg_BackgroundColor, &QCheckBox::setEnabled);
    connect(m_dlg->kcfg_ShellOpenFileInTabs, &QCheckBox::toggled, m_dlg->kcfg_SwitchToTabIfOpen, &QCheckBox::setEnabled);
}

DlgGeneral::~DlgGeneral()
{
    delete m_dlg;
}

void DlgGeneral::showEvent(QShowEvent *)
{
#if OKULAR_FORCE_DRM
    m_dlg->kcfg_ObeyDRM->hide();
#else
    if (KAuthorized::authorize(QStringLiteral("skip_drm")))
        m_dlg->kcfg_ObeyDRM->show();
    else
        m_dlg->kcfg_ObeyDRM->hide();
#endif
}

TitlebarControlCombobox::TitlebarControlCombobox(QWidget* parent)
    : QComboBox(parent)
    , m_displayDocumentTitle(new KcfgPropertyProxy(QStringLiteral("kcfg_DisplayDocumentTitle"), this))
    , m_displayDocumentNameOrPath(new KcfgPropertyProxy(QStringLiteral("kcfg_DisplayDocumentNameOrPath"),this))
{
    addItem(i18nc("@item:inlistbox Config dialog, general page", "Document file name"));
    addItem(i18nc("@item:inlistbox Config dialog, general page", "Document file path"));
    addItem(i18nc("@item:inlistbox Config dialog, general page", "Document title or file name"));
    addItem(i18nc("@item:inlistbox Config dialog, general page", "Document title or file path"));

    connect(this, qOverload<int>(&QComboBox::currentIndexChanged), this, &TitlebarControlCombobox::slotIndexChanged);

    connect(m_displayDocumentTitle, &KcfgPropertyProxy::user_proxyPropertyChanged, this, &TitlebarControlCombobox::slotConfigChanged);
    connect(m_displayDocumentNameOrPath, &KcfgPropertyProxy::user_proxyPropertyChanged, this, &TitlebarControlCombobox::slotConfigChanged);

    setCurrentIndex(0);
    slotIndexChanged(0);
}

void TitlebarControlCombobox::slotIndexChanged(int index)
{
    if (index == 0) {
        m_displayDocumentTitle->user_setProxyProperty(false);
        m_displayDocumentNameOrPath->user_setProxyProperty(Okular::Settings::EnumDisplayDocumentNameOrPath::Name);
    } else if (index == 1) {
        m_displayDocumentTitle->user_setProxyProperty(false);
        m_displayDocumentNameOrPath->user_setProxyProperty(Okular::Settings::EnumDisplayDocumentNameOrPath::Path);
    } else if (index == 2) {
        m_displayDocumentTitle->user_setProxyProperty(true);
        m_displayDocumentNameOrPath->user_setProxyProperty(Okular::Settings::EnumDisplayDocumentNameOrPath::Name);
    } else {
        m_displayDocumentTitle->user_setProxyProperty(true);
        m_displayDocumentNameOrPath->user_setProxyProperty(Okular::Settings::EnumDisplayDocumentNameOrPath::Path);
    }
}

void TitlebarControlCombobox::slotConfigChanged()
{
    if (m_displayDocumentTitle->proxyPropertyValue() == false) {
        if (m_displayDocumentNameOrPath->proxyPropertyValue() == Okular::Settings::EnumDisplayDocumentNameOrPath::Name) {
            setCurrentIndex(0);
        } else {
            setCurrentIndex(1);
        }
    } else {
        if (m_displayDocumentNameOrPath->proxyPropertyValue() == Okular::Settings::EnumDisplayDocumentNameOrPath::Name) {
            setCurrentIndex(2);
        } else {
            setCurrentIndex(3);
        }
    }
}
