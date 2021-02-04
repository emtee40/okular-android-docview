/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgaccessibility.h"

#include "settings.h"

#include <KColorButton>
#include <KLocalizedString>
#include <KMessageWidget>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStackedWidget>

#ifdef HAVE_SPEECH
#include <QtTextToSpeech>
#endif

DlgAccessibility::DlgAccessibility(QWidget *parent)
    : QWidget(parent)
    , m_colorModeConfigStack(new QStackedWidget(this))
{
    QFormLayout *layout = new QFormLayout(this);

    // BEGIN Checkboxes: draw border around images/links
    // ### not working yet, hide for now
    // QCheckBox *highlightImages = new QCheckBox(this);
    // highlightImages->setText(i18nc("@option:check Config dialog, accessibility page", "Draw border around images"));
    // highlightImages->setObjectName(QStringLiteral("kcfg_HighlightImages"));
    // layout->addRow(QString(), highlightImages);

    QCheckBox *highlightLinks = new QCheckBox(this);
    highlightLinks->setText(i18nc("@option:check Config dialog, accessibility page", "Draw border around links"));
    highlightLinks->setObjectName(QStringLiteral("kcfg_HighlightLinks"));
    layout->addRow(QString(), highlightLinks);
    // END Checkboxes: draw border around images/links

    layout->addRow(new QLabel(this));

    // BEGIN Change colors section
    // Checkbox: enable Change Colors feature
    m_enableChangeColors = new QCheckBox(this);
    m_enableChangeColors->setText(i18nc("@option:check Config dialog, accessibility page", "Change colors"));
    m_enableChangeColors->setObjectName(QStringLiteral("kcfg_ChangeColors"));
    layout->addRow(QString(), m_enableChangeColors);

    // Combobox: color modes
    m_colorMode = new QComboBox(this);
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert colors"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Change paper color"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Change dark & light colors"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Convert to black & white"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert lightness"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert luma (sRGB linear)"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert luma (symmetric)"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Shift hue positive"));
    m_colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Shift hue negative"));
    m_colorMode->setObjectName(QStringLiteral("kcfg_RenderMode"));
    layout->addRow(i18nc("@label:listbox Config dialog, accessibility page", "Color mode:"), m_colorMode);

    m_colorModeConfigStack->setSizePolicy({QSizePolicy::Preferred, QSizePolicy::Fixed});

    // Message widget: Performance warning
    m_warningMessage = new KMessageWidget(i18nc("@info Config dialog, accessibility page", "This option can badly affect drawing speed."), this);
    m_warningMessage->setMessageType(KMessageWidget::Warning);
    layout->addRow(m_warningMessage);

    // BEGIN Empty page (Only needed to hide the other pages, but it shouldn’t be huge...)
    QWidget *pageWidget = new QWidget(this);
    QFormLayout *pageLayout = new QFormLayout(pageWidget);
    m_colorModeConfigStack->addWidget(pageWidget);
    // END Empty page

    // BEGIN Change paper color page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Color button: paper color
    KColorButton *paperColor = new KColorButton(this);
    paperColor->setObjectName(QStringLiteral("kcfg_PaperColor"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Paper color:"), paperColor);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Change paper color page

    // BEGIN Change to dark & light colors page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Color button: dark color
    KColorButton *darkColor = new KColorButton(this);
    darkColor->setObjectName(QStringLiteral("kcfg_RecolorForeground"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Dark color:"), darkColor);

    // Color button: light color
    KColorButton *lightColor = new KColorButton(this);
    lightColor->setObjectName(QStringLiteral("kcfg_RecolorBackground"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Light color:"), lightColor);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Change to dark & light colors page

    // BEGIN Convert to black & white page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Slider: threshold
    QSlider *thresholdSlider = new QSlider(this);
    thresholdSlider->setMinimum(2);
    thresholdSlider->setMaximum(253);
    thresholdSlider->setOrientation(Qt::Horizontal);
    thresholdSlider->setObjectName(QStringLiteral("kcfg_BWThreshold"));
    pageLayout->addRow(i18nc("@label:slider Config dialog, accessibility page", "Threshold:"), thresholdSlider);

    // Slider: contrast
    QSlider *contrastSlider = new QSlider(this);
    contrastSlider->setMinimum(2);
    contrastSlider->setMaximum(6);
    contrastSlider->setOrientation(Qt::Horizontal);
    contrastSlider->setObjectName(QStringLiteral("kcfg_BWContrast"));
    pageLayout->addRow(i18nc("@label:slider Config dialog, accessibility page", "Contrast:"), contrastSlider);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Convert to black & white page

    layout->addRow(QString(), m_colorModeConfigStack);

    // Setup controls enabled states:
    m_colorMode->setCurrentIndex(0);
    slotColorModeSelected();
    connect(m_colorMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &DlgAccessibility::slotColorModeSelected);

    m_enableChangeColors->setChecked(false);
    m_colorMode->setEnabled(false);
    connect(m_enableChangeColors, &QCheckBox::toggled, m_colorMode, &QComboBox::setEnabled);
    m_colorModeConfigStack->setEnabled(false);
    connect(m_enableChangeColors, &QCheckBox::toggled, m_colorModeConfigStack, &QWidget::setEnabled);
    connect(m_enableChangeColors, &QCheckBox::toggled, this, &DlgAccessibility::slotColorModeSelected);
    // END Change colors section

#ifdef HAVE_SPEECH
    layout->addRow(new QLabel(this));

    // BEGIN Text-to-speech section
    QComboBox *ttsEngine = new QComboBox(this);
    // Populate tts engines and use their names directly as key and item text:
    const QStringList engines = QTextToSpeech::availableEngines();
    for (const QString &engine : engines) {
        ttsEngine->addItem(engine);
    }
    ttsEngine->setProperty("kcfg_property", QByteArray("currentText"));
    ttsEngine->setObjectName(QStringLiteral("kcfg_ttsEngine"));
    layout->addRow(i18nc("@label:listbox Config dialog, accessibility page", "Text-to-speech engine:"), ttsEngine);
    // END Text-to-speech section
#endif
}

void DlgAccessibility::slotColorModeSelected()
{
    int mode = m_colorMode->currentIndex();

    if (m_enableChangeColors->isChecked() && mode != Okular::Settings::EnumRenderMode::Paper) {
        m_warningMessage->animatedShow();
    } else {
        m_warningMessage->animatedHide();
    }

    if (mode == Okular::Settings::EnumRenderMode::Paper) {
        m_colorModeConfigStack->setCurrentIndex(1);
    } else if (mode == Okular::Settings::EnumRenderMode::Recolor) {
        m_colorModeConfigStack->setCurrentIndex(2);
    } else if (mode == Okular::Settings::EnumRenderMode::BlackWhite) {
        m_colorModeConfigStack->setCurrentIndex(3);
    } else {
        m_colorModeConfigStack->setCurrentIndex(0);
    }
}
