/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QTest>

#include "../part/signaturepartutils.h"

class SuggestedFileNameTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSuggestedSignedDocumentName();
    void testSuggestedSignedDocumentName_data();
};

void SuggestedFileNameTest::testSuggestedSignedDocumentName()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    auto output = SignaturePartUtils::getSuggestedFileNameForSignedFile(input, QStringLiteral("pdf"));
    QCOMPARE(output, expected);
}

void SuggestedFileNameTest::testSuggestedSignedDocumentName_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    // note that we expect english for the tests. The _signed parts might be translated
    QTest::newRow("simple") << QStringLiteral("foo.pdf") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("double extensions") << QStringLiteral("foo.pdf.gz") << QStringLiteral("foo_signed.pdf"); // while we might read compressed files, we don't write  them out
    QTest::newRow("versioning") << QStringLiteral("foo-1.2.3.pdf") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("versioned and double extensions") << QStringLiteral("foo-1.2.3.pdf.gz") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("gif") << QStringLiteral("foo.gif") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("version gif") << QStringLiteral("foo-1.2.3.gif") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("no extension") << QStringLiteral("foo") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("no extension with versions") << QStringLiteral("foo-1.2.3") << QStringLiteral("foo-1.2_signed.pdf"); // This is not as such expected behavior but more a documentation of implementation.
}

QTEST_GUILESS_MAIN(SuggestedFileNameTest)

#include "suggestedfilenametest.moc"
