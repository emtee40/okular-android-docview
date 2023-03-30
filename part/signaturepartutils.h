/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPARTUTILS_H
#define OKULAR_SIGNATUREPARTUTILS_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <memory>
#include <optional>

#include "gui/signatureguiutils.h"

class PageView;
class QComboBox;

namespace SignaturePartUtils
{
struct SigningInformation {
    std::unique_ptr<Okular::CertificateInfo> certificate;
    QString certificatePassword;
    QString documentPassword;
};

/** Retrieves signing information for this operation
    \return nullopt on failure, otherwise information
 */
std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc);

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
    QComboBox *combo;

    explicit SelectCertificateDialog(QWidget *parent);
};
}

#endif
