/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGGENERAL_H
#define _DLGGENERAL_H

#include <QComboBox>
#include <QWidget>

#include "part.h"

class Ui_DlgGeneralBase;

class DlgGeneral : public QWidget
{
    Q_OBJECT

public:
    explicit DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode);
    ~DlgGeneral() override;

protected:
    void showEvent(QShowEvent *) override;

    Ui_DlgGeneralBase *m_dlg;
};

/**
 * Proxy for a single kcfg_... property.
 *
 * KConfigDialogManager interfaces kcfg_... methods,
 * user interfaces user_... methods.
 */
class KcfgPropertyProxy : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QVariant proxyProperty READ proxyPropertyValue WRITE kcfg_setProxyProperty NOTIFY kcfg_proxyPropertyChanged USER true)

protected:
    QVariant m_proxyPropertyValue;

public:
    /**
     * @param propertyName How your property is called, something like @c "kcfg_ThisIsMyConfigKey".
     */
    explicit KcfgPropertyProxy(const QString &objectName, QWidget *parent)
        : QWidget(parent)
    {
        setObjectName(objectName);
    }

    /**
     * The current value of your property.
     */
    QVariant proxyPropertyValue()
    {
        return m_proxyPropertyValue;
    }

Q_SIGNALS:
    /**
     * This is emitted when the property changes in the configuration.
     */
    void user_proxyPropertyChanged();

    /**
     * KConfigDialogManager listens to this; don’t call yourself.
     */
    void kcfg_proxyPropertyChanged();

public Q_SLOTS:
    /**
     * Call this to pass a new value to the configuration.
     */
    void user_setProxyProperty(const QVariant &v)
    {
        if (m_proxyPropertyValue != v) {
            m_proxyPropertyValue = v;
            emit kcfg_proxyPropertyChanged();
        }
    }

    /**
     * KConfigDialogManager uses this to give you new values; don’t call yourself.
     */
    void kcfg_setProxyProperty(const QVariant &v)
    {
        if (m_proxyPropertyValue != v) {
            m_proxyPropertyValue = v;
            emit user_proxyPropertyChanged();
        }
    }
};

/**
 * Bundles kcfg_DisplayDocumentTitle and kcfg_DisplayDocumentNameOrPath in one combobox.
 */
class TitlebarControlCombobox : public QComboBox
{
    Q_OBJECT

public:
    explicit TitlebarControlCombobox(QWidget *parent);

protected:
    /** bool DisplayDocumentTitle */
    KcfgPropertyProxy *m_displayDocumentTitle;
    /** enum int DisplayDocumentNameOrPath */
    KcfgPropertyProxy *m_displayDocumentNameOrPath;

protected Q_SLOTS:
    void slotIndexChanged(int index);
    void slotConfigChanged();
};

#endif
