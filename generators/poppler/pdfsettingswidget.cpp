/*
    SPDX-FileCopyrightText: 2019 Bubli <Katarina.Behrens@cib.de>
    SPDX-FileCopyrightText: 2020 Albert Astals Cid <albert.astals.cid@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pdfsettingswidget.h"

#include "pdfsettings.h"
#include "pdfsignatureutils.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 05, 0)
QString PDFSettingsWidget::popplerEnumToSettingString(Poppler::SignatureBackend backend)
{
    switch (backend) {
    case Poppler::SignatureBackend::NSS:
        return QStringLiteral("NSS");
    case Poppler::SignatureBackend::GPG:
        return QStringLiteral("GPG");
    case Poppler::SignatureBackend::None: // should not be user visible
        return {};
    }
    return {};
}

static QString popplerEnumToUserString(Poppler::SignatureBackend backend)
{
    // I'm unsure if we want these translatable, but if so
    // we sholud do something here rather than forward directly to the
    // technical popplerEnumToSettingString
    return PDFSettingsWidget::popplerEnumToSettingString(backend);
}

Poppler::SignatureBackend PDFSettingsWidget::settingStringToPopplerEnum(QStringView backend)
{
    if (backend == QStringLiteral("NSS"))
        return Poppler::SignatureBackend::NSS;
    if (backend == QStringLiteral("GPG"))
        return Poppler::SignatureBackend::GPG;
    return Poppler::SignatureBackend::None;
}
#endif

PDFSettingsWidget::PDFSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pdfsw.setupUi(this);

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 05, 0)
    auto backends = Poppler::availableBackends();
    if (!backends.empty()) {
        // Let's try get the currently stored backend:
        auto currentBackend = settingStringToPopplerEnum(PDFSettings::self()->signatureBackend());
        if (currentBackend == Poppler::SignatureBackend::None) {
            currentBackend = Poppler::activeBackend();
        } else if (currentBackend != Poppler::activeBackend()) {
            if (!Poppler::setActiveBackend(currentBackend)) {
                // erm. This must be a case of having either modified
                // the config file manually to something not available
                // in the poppler installed here or have reconfigured
                // their poppler to not have the previously selected one
                // available any longer.
                // Probably the safest bet is to take whatever is active
                currentBackend = Poppler::activeBackend();
            }
        }
        int selected = -1;
        for (auto backend : backends) {
            if (backend == currentBackend) {
                selected = m_pdfsw.kcfg_SignatureBackend->count();
            }
            m_pdfsw.kcfg_SignatureBackend->addItem(popplerEnumToUserString(backend), QVariant(popplerEnumToSettingString(backend)));
        }
        m_pdfsw.kcfg_SignatureBackend->setProperty("kcfg_property", QByteArray("currentData"));

        m_pdfsw.kcfg_SignatureBackend->setCurrentIndex(selected);
        connect(m_pdfsw.kcfg_SignatureBackend, &QComboBox::currentTextChanged, [this](const QString &text) {
            auto backendEnum = settingStringToPopplerEnum(text);
            if (backendEnum == Poppler::SignatureBackend::None) {
                return;
            }
            Poppler::setActiveBackend(backendEnum);
            m_pdfsw.certDBGroupBox->setVisible(backendEnum == Poppler::SignatureBackend::NSS);
            m_certificatesAsked = false;
            if (m_tree) {
                m_tree->clear();
            }
            update();
        });

        m_pdfsw.certDBGroupBox->setVisible(currentBackend == Poppler::SignatureBackend::NSS);
#else
    if (Poppler::hasNSSSupport()) {
        // Better hide the signature backend selection; we have not really any
        // need for that.
        for (int i = 0; i < m_pdfsw.signatureBackendLayout->count(); i++) {
            auto item = m_pdfsw.signatureBackendLayout->itemAt(i);
            if (auto widget = item->widget()) {
                widget->hide();
            }
        }
#endif

        m_pdfsw.loadSignaturesButton->hide();

        KUrlRequester *pDlg = new KUrlRequester();
        pDlg->setObjectName(QStringLiteral("kcfg_DBCertificatePath"));
        pDlg->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
        pDlg->setEnabled(false);
        m_pdfsw.formLayout->setWidget(1, QFormLayout::FieldRole, pDlg);

        connect(m_pdfsw.customRadioButton, &QRadioButton::toggled, pDlg, &KUrlRequester::setEnabled);

        if (!PDFSettings::useDefaultCertDB()) {
            m_pdfsw.customRadioButton->setChecked(true);
            m_pdfsw.defaultLabel->setVisible(false);
        }

        m_tree = new QTreeWidget(this);
        m_tree->setHeaderLabels({i18nc("Name of the person to whom the cerficate was issued", "Issued to"), i18n("E-mail"), i18nc("Certificate expiration date", "Expiration date")});
        m_tree->setRootIsDecorated(false);

        m_pdfsw.certificatesPlaceholder->addWidget(m_tree);

        connect(PDFSettings::self(), &PDFSettings::useDefaultDBChanged, this, &PDFSettingsWidget::warnRestartNeeded);
        connect(PDFSettings::self(), &PDFSettings::dBCertificatePathChanged, this, [this] {
            if (!PDFSettings::useDefaultCertDB()) {
                warnRestartNeeded();
            }
        });
        connect(m_pdfsw.loadSignaturesButton, &QPushButton::clicked, this, [this] {
            m_certificatesAsked = false;
            update();
        });
    } else {
        QHBoxLayout *lay = new QHBoxLayout(this);
        QLabel *l = new QLabel(i18n("You are using a Poppler library built without NSS support.\nAdding Digital Signatures isn't available for that reason"));
        l->setWordWrap(true);
        lay->addWidget(l);
    }
}

bool PDFSettingsWidget::event(QEvent *e)
{
    if (m_tree && e->type() == QEvent::Paint && !m_certificatesAsked) {
        m_certificatesAsked = true;

        PopplerCertificateStore st;
        bool userCancelled;
        const QList<Okular::CertificateInfo *> certs = st.signingCertificates(&userCancelled);

        m_pdfsw.loadSignaturesButton->setVisible(userCancelled);

        for (auto cert : certs) {
            new QTreeWidgetItem(m_tree,
                                {cert->subjectInfo(Okular::CertificateInfo::EntityInfoKey::CommonName), cert->subjectInfo(Okular::CertificateInfo::EntityInfoKey::EmailAddress), cert->validityEnd().toString(QStringLiteral("yyyy-MM-dd"))});
        }
        qDeleteAll(certs);

        m_pdfsw.defaultLabel->setText(Poppler::getNSSDir());

        m_tree->resizeColumnToContents(1);
        m_tree->resizeColumnToContents(0);
    }
    return QWidget::event(e);
}

void PDFSettingsWidget::warnRestartNeeded()
{
    if (!m_warnedAboutRestart) {
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 05, 0)
        if (PDFSettings::self()->signatureBackend() != QStringLiteral("NSS")) {
            return;
        }
#endif
        m_warnedAboutRestart = true;
        QMessageBox::information(this, i18n("Restart needed"), i18n("You need to restart Okular after changing the NSS directory settings"));
    }
}
