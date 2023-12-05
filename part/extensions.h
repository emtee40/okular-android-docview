/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <kparts/browserextension.h>
#include <kparts/liveconnectextension.h>
using NavigationExtension = KParts::BrowserExtension;
#else
#include <KParts/NavigationExtension>
using NavigationExtension = KParts::NavigationExtension;
#endif

namespace Okular
{
class Part;

class BrowserExtension : public NavigationExtension
{
    Q_OBJECT

public:
    explicit BrowserExtension(Part *);

public Q_SLOTS:
    // Automatically detected by the host.
    void print();

private:
    Part *m_part;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class OkularLiveConnectExtension : public KParts::LiveConnectExtension
{
    Q_OBJECT

public:
    explicit OkularLiveConnectExtension(Part *parent);

    // from LiveConnectExtension
    bool get(const unsigned long objid, const QString &field, Type &type, unsigned long &retobjid, QString &value) override;
    bool put(const unsigned long objid, const QString &field, const QString &value) override;
    bool call(const unsigned long objid, const QString &func, const QStringList &args, Type &type, unsigned long &retobjid, QString &value) override;

private:
    QString eval(const QString &script);
    void postMessage(const QStringList &args);

    bool m_inEval;
    QString m_evalRes;
};
#endif

}

#endif

/* kate: replace-tabs on; indent-width 4; */
