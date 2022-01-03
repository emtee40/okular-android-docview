/*
    SPDX-FileCopyrightText: 2022 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTest>
#include <QTimer>

#include "../core/document.h"
#include "../core/form.h"
#include "../core/page.h"
#include "../settings_core.h"

class EnterPasswordDialogHelper : public QObject
{
    Q_OBJECT

public:
    EnterPasswordDialogHelper()
    {
        QTimer::singleShot(0, this, &EnterPasswordDialogHelper::enterPassword);
    }

    void enterPassword()
    {
        QWidget *dialog = qApp->activeModalWidget();
        if (!dialog) {
            QTimer::singleShot(0, this, &EnterPasswordDialogHelper::enterPassword);
            return;
        }
        QLineEdit *lineEdit = dialog->findChild<QLineEdit *>();
        lineEdit->setText("fakeokular");

        QDialogButtonBox *buttonBox = dialog->findChild<QDialogButtonBox *>();
        buttonBox->button(QDialogButtonBox::Ok)->click();
    }
};

class SignUnsignedFieldTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testSignUnsignedField();

private:
    Okular::Document *m_document;
};

void SignUnsignedFieldTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    Okular::SettingsCore::instance(QStringLiteral("signunsignedfieldtest"));

    KConfig cfg("okular-generator-popplerrc");
    KConfigGroup g = cfg.group(QStringLiteral("Signatures"));
    g.writeEntry(QStringLiteral("UseDefaultCertDB"), false);
    g.writeEntry(QStringLiteral("DBCertificatePath"), "file://" KDESRCDIR "data/fake_okular_certstore");

    m_document = new Okular::Document(nullptr);
}

void SignUnsignedFieldTest::init()
{
    const QString testFile = QStringLiteral(KDESRCDIR "data/hello_with_dummy_signature.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);
}

void SignUnsignedFieldTest::cleanup()
{
    m_document->closeDocument();
}

void SignUnsignedFieldTest::testSignUnsignedField()
{
    const QLinkedList<Okular::FormField *> forms = m_document->page(0)->formFields();
    QCOMPARE(forms.count(), 1);
    Okular::FormFieldSignature *ffs = dynamic_cast<Okular::FormFieldSignature *>(forms.first());

    // This is a hacky way of doing ifdef HAVE_POPPLER_22_02
    if (m_document->metaData(QStringLiteral("CanSignDocumentWithPassword")).toString() == QLatin1String("yes")) {
        QCOMPARE(ffs->signatureType(), Okular::FormFieldSignature::UnsignedSignature);

        const Okular::CertificateStore *certStore = m_document->certificateStore();
        bool userCancelled, nonDateValidCerts;
        {
            EnterPasswordDialogHelper helper;
            const QList<Okular::CertificateInfo *> &certs = certStore->signingCertificatesForNow(&userCancelled, &nonDateValidCerts);
        }

        Okular::NewSignatureData data;
        data.setCertNickname("fake-okular");
        QTemporaryFile f;
        f.open();
        QVERIFY(ffs->sign(data, f.fileName()));

        m_document->closeDocument();
        QMimeDatabase db;
        const QMimeType mime = db.mimeTypeForFile(f.fileName());
        QCOMPARE(m_document->openDocument(f.fileName(), QUrl(), mime), Okular::Document::OpenSuccess);

        const QLinkedList<Okular::FormField *> newForms = m_document->page(0)->formFields();
        QCOMPARE(newForms.count(), 1);
        ffs = dynamic_cast<Okular::FormFieldSignature *>(newForms.first());
        QCOMPARE(ffs->signatureType(), Okular::FormFieldSignature::AdbePkcs7detached);
        QCOMPARE(ffs->signatureInfo().signerName(), "FakeOkular");
    }
}

QTEST_MAIN(SignUnsignedFieldTest)
#include "signunsignedfieldtest.moc"
