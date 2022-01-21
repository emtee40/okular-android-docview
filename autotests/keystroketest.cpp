/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include "../settings_core.h"
#include <QLocale>
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>
#include <core/document.h>
#include <core/form.h>
#include <core/page.h>
#include <qtestcase.h>

#include "../generators/poppler/config-okular-poppler.h"

class KeystrokeTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testCommit();
    void testKeystroke();

private:
    Okular::Document *m_document;
    QMap<QString, Okular::FormField *> m_fields;
};

void KeystrokeTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("keystroketest"));
    m_document = new Okular::Document(nullptr);

    // Force consistent locale
    QLocale locale(QStringLiteral("en_US"));
    QLocale::setDefault(locale);

    const QString testFile = QStringLiteral(KDESRCDIR "data/keystroketest.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    const Okular::Page *page = m_document->page(0);
    const QLinkedList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        m_fields.insert(ff->name(), ff);
    }
}

void KeystrokeTest::testCommit()
{
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields["field2"]);

    // text that will be accepted
    fft->setText("Lorem ipsum");
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "Lorem ipsum", true);
    QCOMPARE(fft->text(), "Lorem ipsum");

    // text that will be rejected
    fft->setText("foo");
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "foo", true);
    QEXPECT_FAIL("", "reset to commited value not implemented", Continue);
    QCOMPARE(fft->text(), "Lorem ipsum");
}

void KeystrokeTest::testKeystroke()
{
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields["field3"]);

    // accept
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "hello", false);
    QCOMPARE(fft->text(), "hello");

    // accept
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "e", false);
    QCOMPARE(fft->text(), "e");

    // accept
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "ee", false);
    QCOMPARE(fft->text(), "ee");

    // accept
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "eee", false);
    QCOMPARE(fft->text(), "eee");

    // reject
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, "eeef", false);
    QCOMPARE(fft->text(), "eee");
}

void KeystrokeTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

QTEST_MAIN(KeystrokeTest)
#include "keystroketest.moc"
