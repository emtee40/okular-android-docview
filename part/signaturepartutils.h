/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPARTUTILS_H
#define OKULAR_SIGNATUREPARTUTILS_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <memory>

#include "gui/signatureguiutils.h"

class PageView;
class QListView;

namespace SignaturePartUtils
{
std::unique_ptr<Okular::CertificateInfo> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, QString *password, QString *documentPassword);
QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc);
void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc);

class KeyDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;
};

class SelectCertificateDialog : public QDialog
{
    Q_OBJECT
public:
    QListView *list;

    explicit SelectCertificateDialog(QWidget *parent);
};
}

#endif
