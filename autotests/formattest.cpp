/***************************************************************************
 *   Copyright (C) 2019 by João Netto <joaonetto901@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <QMap>
#include <QMimeType>
#include <QMimeDatabase>
#include "../settings_core.h"
#include <core/document.h>
#include <core/page.h>
#include <core/form.h>
#include <QLocale>

#include "../generators/poppler/config-okular-poppler.h"

class FormatTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testTimeFormat();
    void testTimeFormat_data();
    void testSpecialFormat();
    void testSpecialFormat_data();
    void testPercentFormat();
    void testPercentFormat_data();
private:

    Okular::Document *m_document;
    QMap<QString, Okular::FormField*> m_fields;
    QString m_formattedText;
};

void FormatTest::initTestCase()
{
    Okular::SettingsCore::instance( QStringLiteral( "formattest" ) );
    m_document = new Okular::Document( nullptr );

    // Force consistent locale
    QLocale locale( QStringLiteral( "en_US" ) );
    QLocale::setDefault( locale );

    const QString testFile = QStringLiteral( KDESRCDIR "data/formattest.pdf" );
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( testFile );
    QCOMPARE( m_document->openDocument( testFile, QUrl(), mime), Okular::Document::OpenSuccess );

    connect( m_document, &Okular::Document::refreshFormWidget, [=]( Okular::FormField * form )
            {
                Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >( form );
                if( fft )
                    m_formattedText = fft->text();
            });

    const Okular::Page* page = m_document->page( 0 );
    for ( Okular::FormField *ff: page->formFields() )
    {
        m_fields.insert( ff->name(),  ff );
    }
}

void FormatTest::testTimeFormat()
{
    QFETCH( QString, fieldName );
    QFETCH( QString, text );
    QFETCH( QString, result );

    Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ fieldName ] );
    fft->setText( text );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( m_formattedText, result );
}

void FormatTest::testTimeFormat_data()
{
    QTest::addColumn< QString >( "fieldName" );
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< QString > ( "result" );

    QTest::newRow( "field hh:mm" ) << QStringLiteral( "time1" ) << QStringLiteral( "1:20" ) << QStringLiteral( "01:20" );
    QTest::newRow( "field hh:mm with pm" ) << QStringLiteral( "time1" ) << QStringLiteral( "1:20 pm" ) << QStringLiteral( "13:20" );
    QTest::newRow( "field hh:mm invalid one number" ) << QStringLiteral( "time1" ) << QStringLiteral( "1" ) << QStringLiteral( "" );
    QTest::newRow( "field hh:mm invalid time" ) << QStringLiteral( "time1" ) << QStringLiteral( "25:12" ) << QStringLiteral( "" );
    QTest::newRow( "field hh:mm invalid only letters" ) << QStringLiteral( "time1" ) << QStringLiteral( "abcd" ) << QStringLiteral( "" );
    QTest::newRow( "field hh:mm ap" ) << QStringLiteral( "time2" ) << QStringLiteral( "1:20" ) << QStringLiteral( "1:20 am" );
    QTest::newRow( "field hh:mm ap remove zero" ) << QStringLiteral( "time2" ) << QStringLiteral( "01:20 pm" ) << QStringLiteral( "1:20 pm" );
    QTest::newRow( "field hh:mm ap change to AM/PM" ) << QStringLiteral( "time2" ) << QStringLiteral( "13:20" ) << QStringLiteral( "1:20 pm" );
    QTest::newRow( "field hh:mm:ss without seconds" ) << QStringLiteral( "time3" ) << QStringLiteral( "1:20" ) << QStringLiteral( "01:20:00" );
    QTest::newRow( "field hh:mm:ss with pm" ) << QStringLiteral( "time3" ) << QStringLiteral( "1:20:00 pm" ) << QStringLiteral( "13:20:00" );
    QTest::newRow( "field hh:mm:ss ap without am" ) << QStringLiteral( "time4" ) << QStringLiteral( "1:20:00" ) << QStringLiteral( "1:20:00 am" );
    QTest::newRow( "field hh:mm:ss ap remove 0" ) << QStringLiteral( "time4" ) << QStringLiteral( "01:20:00 pm" ) << QStringLiteral( "1:20:00 pm" );
    QTest::newRow( "field hh:mm:ss ap change to AM/PM" ) << QStringLiteral( "time4" ) << QStringLiteral( "13:20:00" ) << QStringLiteral( "1:20:00 pm" );
}

void FormatTest::testSpecialFormat()
{
    m_formattedText = QStringLiteral( "" );
    QFETCH( QString, fieldName );
    QFETCH( QString, text );
    QFETCH( bool, edited );
    QFETCH( QString, result );

    Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ fieldName ] );
    fft->setText( text );
    bool ok = false;
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );

    QCOMPARE( m_formattedText, result );
    QCOMPARE( ok, edited );
}

void FormatTest::testSpecialFormat_data()
{
    QTest::addColumn< QString >( "fieldName" );
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< bool >( "edited" );
    QTest::addColumn< QString > ( "result" );

    // The tests which have invalid edited, keep the same value as when it was formatted before.
    QTest::newRow( "field validated but not changed" ) << QStringLiteral( "CEP" ) << QStringLiteral( "12345" ) << true << QStringLiteral( "" );
    QTest::newRow( "field invalid but not changed" ) << QStringLiteral( "CEP" ) << QStringLiteral( "123456" ) << false << QStringLiteral( "" );
    QTest::newRow( "field formatted and changed" ) << QStringLiteral( "8Digits" ) << QStringLiteral( "123456789" ) << true << QStringLiteral( "12345-6789" );
    QTest::newRow( "field invalid 10 digits" ) << QStringLiteral( "8Digits" ) << QStringLiteral( "1234567890" ) << false << QStringLiteral( "12345-6789" );
    QTest::newRow( "field formatted telephone" ) << QStringLiteral( "telefone" ) << QStringLiteral( "1234567890" ) << true << QStringLiteral( "(123) 456-7890" );
    QTest::newRow( "field invalid telephone" ) << QStringLiteral( "telefone" ) << QStringLiteral( "12345678900" ) << false << QStringLiteral( "(123) 456-7890" );
    QTest::newRow( "field formmated SSN" ) << QStringLiteral( "CPF" ) << QStringLiteral( "123456789" ) << true << QStringLiteral( "123-45-6789" );
    QTest::newRow( "field invalid SSN" ) << QStringLiteral( "CPF" ) << QStringLiteral( "1234567890" ) << false << QStringLiteral( "123-45-6789" );
}

void FormatTest::testPercentFormat()
{
    m_formattedText = QStringLiteral( "" );
    QFETCH( QString, fieldName );
    QFETCH( QString, text );
    QFETCH( bool, edited );
    QFETCH( QString, result );

    Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ fieldName ] );
    bool ok = false;
    fft->setText( text );
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok, true );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );
    
    QCOMPARE( ok, edited );
    QCOMPARE( m_formattedText, result );
}

void FormatTest::testPercentFormat_data()
{
    QTest::addColumn< QString >( "fieldName" );
    QTest::addColumn< QString >( "text" );
    QTest::addColumn< bool >( "edited" );
    QTest::addColumn< QString > ( "result" );

    // The tests which have invalid edited, keep the same value as when it was formatted before.
    QTest::newRow( "normal percent" ) << QStringLiteral( "pct1" ) << QStringLiteral( "1.20" ) << true << QStringLiteral( "120.00 %" );
    QTest::newRow( "percent with comma thousands sep" ) << QStringLiteral( "pct1" ) << QStringLiteral( "1234.20" ) << true << QStringLiteral( "123,420.00 %" );
    QTest::newRow( "invalid number" ) << QStringLiteral( "pct1" ) << QStringLiteral( "1234,20" ) << false << QStringLiteral( "" );
    QTest::newRow( "normal percent 2" ) << QStringLiteral( "pct2" ) << QStringLiteral( "1.20" ) << true << QStringLiteral( "120.00 %" );
    QTest::newRow( "percent without comma thousands sep" ) << QStringLiteral( "pct2" ) << QStringLiteral( "1234.20" ) << true << QStringLiteral( "123420.00 %" );
    QTest::newRow( "percent with comma dot sep" ) << QStringLiteral( "pct3" ) << QStringLiteral( "1,20" ) << true << QStringLiteral( "120,00 %" );
    QTest::newRow( "percent with comma dot sep and thousands dot sep" ) << QStringLiteral( "pct3" ) << QStringLiteral( "1234,20" ) << true << QStringLiteral( "123.420,00 %" );
    QTest::newRow( "invalid number with dot sep" ) << QStringLiteral( "pct3" ) << QStringLiteral( "1234.20" ) << false << QStringLiteral( "" );
    QTest::newRow( "normal percent 3" ) << QStringLiteral( "pct4" ) << QStringLiteral( "1,20" ) << true << QStringLiteral( "120,00 %" );
    QTest::newRow( "normal percent 4 with \' as sep" ) << QStringLiteral( "pct4" ) << QStringLiteral( "1234,20" ) << true << QStringLiteral( "123420,00 %" );
}


void FormatTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}


QTEST_MAIN( FormatTest )
#include "formattest.moc"
