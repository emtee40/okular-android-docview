/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificatetools.h"
#include "certsettings.h"
#include <iostream>
#include <klocalizedstring.h>

#include <poppler-form.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

CertificateTools::CertificateTools(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    m_list = new QListWidget(this);
    hBoxLayout->addWidget(m_list);

    const QVector<Poppler::CertificateInfo> nssCerts = Poppler::getAvailableSigningCertificates();
    foreach (auto cert, nssCerts) {
        QListWidgetItem *listEntry = new QListWidgetItem(
            cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::CommonName) + "\t\t" + cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::EmailAddress) + "\t\t(" + cert.validityEnd().toString("yyyy-MM-dd") + ")", m_list);
    }
}

CertificateTools::~CertificateTools()
{
}
