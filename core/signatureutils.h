/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREUTILS_H
#define OKULAR_SIGNATUREUTILS_H

#include "okularcore_export.h"

#include <QDateTime>
#include <QFlag>
#include <QList>
#include <QSharedPointer>
#include <QString>

namespace Okular
{

/**
 * @short A helper class to store information about x509 certificate
 */
class CertificateInfoPrivate;
class OKULARCORE_EXPORT CertificateInfo
{
public:
    /**
     * The algorithm of public key.
     */
    enum PublicKeyType { RsaKey, DsaKey, EcKey, OtherKey };

    /**
     * Certificate key usage extensions.
     */
    enum KeyUsageExtension { KuDigitalSignature = 0x80, KuNonRepudiation = 0x40, KuKeyEncipherment = 0x20, KuDataEncipherment = 0x10, KuKeyAgreement = 0x08, KuKeyCertSign = 0x04, KuClrSign = 0x02, KuEncipherOnly = 0x01, KuNone = 0x00 };
    Q_DECLARE_FLAGS(KeyUsageExtensions, KeyUsageExtension)

    /**
     * Predefined keys for elements in an entity's distinguished name.
     */
    enum EntityInfoKey {
        CommonName,
        DistinguishedName,
        EmailAddress,
        Organization,
    };

    /**
     * Destructor
     */
    ~CertificateInfo();

    /**
     * The certificate version string.
     */
    int version() const;
    void setVersion(int version);

    /**
     * The certificate serial number.
     */
    QByteArray serialNumber() const;
    void setSerialNumber(const QByteArray &serial);

    /**
     * Information about the issuer.
     */
    QString issuerInfo(EntityInfoKey key) const;
    void setIssuerInfo(EntityInfoKey key, const QString &info);

    /**
     * Information about the subject
     */
    QString subjectInfo(EntityInfoKey key) const;
    void setSubjectInfo(EntityInfoKey key, const QString &info);

    /**
     * The certificate internal database nickname
     */
    QString nickName() const;
    void setNickName(const QString &nickName);

    /**
     * The date-time when certificate becomes valid.
     */
    QDateTime validityStart() const;
    void setValidityStart(const QDateTime &start);

    /**
     * The date-time when certificate expires.
     */
    QDateTime validityEnd() const;
    void setValidityEnd(const QDateTime &validityEnd);

    /**
     * The uses allowed for the certificate.
     */
    KeyUsageExtensions keyUsageExtensions() const;
    void setKeyUsageExtensions(KeyUsageExtensions ext);

    /**
     * The public key value.
     */
    QByteArray publicKey() const;
    void setPublicKey(const QByteArray &publicKey);

    /**
     * The public key type.
     */
    PublicKeyType publicKeyType() const;
    void setPublicKeyType(PublicKeyType type);

    /**
     * The strength of public key in bits.
     */
    int publicKeyStrength() const;
    void setPublicKeyStrength(int strength);

    /**
     * Returns true if certificate is self-signed otherwise returns false.
     */
    bool isSelfSigned() const;
    void setSelfSigned(bool selfSigned);

    /**
     * The DER encoded certificate.
     */
    QByteArray certificateData() const;
    void setCertificateData(const QByteArray &certificateData);

    /**
     * Checks if the given password is the correct one for this certificate
     *
     * @since 21.04
     */
    bool checkPassword(const QString &password) const;
    void setCheckPasswordFunction(const std::function<bool(const QString &)> &passwordFunction);

    CertificateInfo();
    CertificateInfo(const CertificateInfo &other);
    CertificateInfo(CertificateInfo &&other);
    CertificateInfo &operator=(const CertificateInfo &other);
    CertificateInfo &operator=(CertificateInfo &&other);

private:
    QSharedDataPointer<CertificateInfoPrivate> d;
};

/**
 * @short A helper class to store information about digital signature
 */
class SignatureInfoPrivate;
class OKULARCORE_EXPORT SignatureInfo
{
public:
    /**
     * The verification result of the signature.
     */
    enum SignatureStatus {
        SignatureStatusUnknown,  ///< The signature status is unknown for some reason.
        SignatureValid,          ///< The signature is cryptographically valid.
        SignatureInvalid,        ///< The signature is cryptographically invalid.
        SignatureDigestMismatch, ///< The document content was changed after the signature was applied.
        SignatureDecodingError,  ///< The signature CMS/PKCS7 structure is malformed.
        SignatureGenericError,   ///< The signature could not be verified.
        SignatureNotFound,       ///< The requested signature is not present in the document.
        SignatureNotVerified     ///< The signature is not yet verified.
    };

    /**
     * The verification result of the certificate.
     */
    enum CertificateStatus {
        CertificateStatusUnknown,   ///< The certificate status is unknown for some reason.
        CertificateTrusted,         ///< The certificate is considered trusted.
        CertificateUntrustedIssuer, ///< The issuer of this certificate has been marked as untrusted by the user.
        CertificateUnknownIssuer,   ///< The certificate trust chain has not finished in a trusted root certificate.
        CertificateRevoked,         ///< The certificate was revoked by the issuing certificate authority.
        CertificateExpired,         ///< The signing time is outside the validity bounds of this certificate.
        CertificateGenericError,    ///< The certificate could not be verified.
        CertificateNotVerified      ///< The certificate is not yet verified.
    };

    /**
     * The hash algorithm of the signature
     */
    enum HashAlgorithm { HashAlgorithmUnknown, HashAlgorithmMd2, HashAlgorithmMd5, HashAlgorithmSha1, HashAlgorithmSha256, HashAlgorithmSha384, HashAlgorithmSha512, HashAlgorithmSha224 };

    /**
     * Destructor.
     */
    ~SignatureInfo();

    /**
     * The signature status of the signature.
     */
    SignatureStatus signatureStatus() const;
    void setSignatureStatus(SignatureStatus status);

    /**
     * The certificate status of the signature.
     */
    CertificateStatus certificateStatus() const;
    void setCertificateStatus(CertificateStatus status);

    /**
     * The signer subject common name associated with the signature.
     */
    QString signerName() const;
    void setSignerName(const QString &signerName);

    /**
     * The signer subject distinguished name associated with the signature.
     */
    QString signerSubjectDN() const;
    void setSignerSubjectDN(const QString &signerSubjectDN);

    /**
     * Get signing location.
     */
    QString location() const;
    void setLocation(const QString &location);

    /**
     * Get signing reason.
     */
    QString reason() const;
    void setReason(const QString &reason);

    /**
     * The hash algorithm used for the signature.
     */
    HashAlgorithm hashAlgorithm() const;
    void setHashAlgorithm(HashAlgorithm algorithm);

    /**
     * The signing time associated with the signature.
     */
    QDateTime signingTime() const;
    void setSigningTime(const QDateTime &time);

    /**
     * Get the signature binary data.
     */
    QByteArray signature() const;
    void setSignature(const QByteArray &signature);

    /**
     * Get the bounds of the ranges of the document which are signed.
     */
    QList<qint64> signedRangeBounds() const;
    void setSignedRangeBounds(const QList<qint64> &range);

    /**
     * Checks whether the signature authenticates the total document
     * except for the signature itself.
     */
    bool signsTotalDocument() const;
    void setSignsTotalDocument(bool total);

    /**
     * Get certificate details.
     */
    CertificateInfo certificateInfo() const;
    void setCertificateInfo(const CertificateInfo &other);

    SignatureInfo();
    SignatureInfo(const SignatureInfo &other);
    SignatureInfo(SignatureInfo &&other);
    SignatureInfo &operator=(const SignatureInfo &other);
    SignatureInfo &operator=(SignatureInfo &&other);

private:
    QSharedDataPointer<SignatureInfoPrivate> d;
};

/**
 * @short A helper class to store information about x509 certificate
 */
class OKULARCORE_EXPORT CertificateStore
{
public:
    /**
     * Destructor
     */
    virtual ~CertificateStore();

    /**
     * Returns list of valid, usable signing certificates.
     *
     * This can ask the user for a password, userCancelled will be true if the user decided not to enter it.
     */
    virtual QList<CertificateInfo> signingCertificates(bool *userCancelled) const;

    /**
     * Returns list of valid, usable signing certificates for current date and time.
     *
     * This can ask the user for a password, userCancelled will be true if the user decided not to enter it.
     *
     * nonDateValidCerts is true if the user has signing certificates but their validity start date is in the future or past their validity end date.
     */
    QList<CertificateInfo> signingCertificatesForNow(bool *userCancelled, bool *nonDateValidCerts) const;

protected:
    CertificateStore();

private:
    Q_DISABLE_COPY(CertificateStore)
};

}

#endif
