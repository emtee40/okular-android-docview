/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_
#define _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_

#include <poppler-form.h>

#include "core/signatureutils.h"

#include <config-okular-poppler.h>

class PopplerCertificateInfo : public Okular::CertificateInfo
{
public:
    PopplerCertificateInfo(const Poppler::CertificateInfo &info);
    ~PopplerCertificateInfo() override;

    bool isNull() const override;
    int version() const override;
    QByteArray serialNumber() const override;
    QString issuerInfo(EntityInfoKey) const override;
    QString subjectInfo(EntityInfoKey) const override;
    QString nickName() const override;
    QDateTime validityStart() const override;
    QDateTime validityEnd() const override;
    KeyUsageExtensions keyUsageExtensions() const override;
    QByteArray publicKey() const override;
    PublicKeyType publicKeyType() const override;
    int publicKeyStrength() const override;
    bool isSelfSigned() const override;
    QByteArray certificateData() const override;
    bool checkPassword(const QString &password) const override;

private:
    Poppler::CertificateInfo m_info;
};

class PopplerSignatureInfo : public Okular::SignatureInfo
{
public:
    PopplerSignatureInfo(const Poppler::SignatureValidationInfo &info);
    ~PopplerSignatureInfo() override;

    SignatureStatus signatureStatus() const override;
    CertificateStatus certificateStatus() const override;
    QString signerName() const override;
    QString signerSubjectDN() const override;
    QString location() const override;
    QString reason() const override;
    HashAlgorithm hashAlgorithm() const override;
    QDateTime signingTime() const override;
    QByteArray signature() const override;
    QList<qint64> signedRangeBounds() const override;
    bool signsTotalDocument() const override;
    const Okular::CertificateInfo &certificateInfo() const override;

private:
    Poppler::SignatureValidationInfo m_info;
    Okular::CertificateInfo *m_certfiticateInfo;
};

#ifdef HAVE_POPPLER_SIGNING

class PopplerCertificateStore : public Okular::CertificateStore
{
public:
    ~PopplerCertificateStore();

    QList<Okular::CertificateInfo *> signingCertificates(bool *userCancelled) const override;
};

#endif

#endif
