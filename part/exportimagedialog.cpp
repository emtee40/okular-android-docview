#include "exportimagedialog.h"

#include <iostream>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPixmap>
#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QString>
#include <QFileDialog>

#include <KLocalizedString>

#include "core/observer.h"
#include "core/document.h"

ExportImageDialog::ExportImageDialog(QWidget *parent, Okular::Document *document, QString *fileName, QString filter)
    : m_parentWidget(parent)
    , m_document(document)
    , m_fileName(fileName)
    , m_filter(filter)
{
    initUI();
}

ExportImageDialog::~ExportImageDialog()
{

}

void ExportImageDialog::initUI()
{
    imageTypeLabel = new QLabel(i18n("Select Image Type:"), this);
    imageTypeComboBox = new QComboBox(this);
    imageTypeComboBox->addItem(i18n("PNG"));
    imageTypeComboBox->addItem(i18n("JPEG"));

    // File Name selection
    fileNameLabel = new QLabel(i18n("Output file:"), this);
    fileNameLineEdit = new QLineEdit(this);
    fileNameBrowseButton = new QPushButton(i18n("..."), this);
    fileNameBrowseButton->setMaximumSize(30, 30);
    connect(fileNameBrowseButton, &QPushButton::clicked, this, &ExportImageDialog::searchFileName);

    // Options tab
    exportRangeGroupBox = new QGroupBox(i18n("Export range"), this);
    qualitySelectorGroupBox = new QGroupBox(i18n("Quality select"), this);

    /// Setup Page Export Ranges
    allPagesRadioButton = new QRadioButton(i18n("Export all"), exportRangeGroupBox);
    allPagesRadioButton->setChecked(true);
    pageRangeRadioButton = new QRadioButton(i18n("Pages from"), exportRangeGroupBox);
    toLabel = new QLabel(i18n("to"), exportRangeGroupBox);
    customPageRadioButton = new QRadioButton(i18n("Pages"), exportRangeGroupBox);
    
    pageStartSpinBox = new QSpinBox(exportRangeGroupBox);
    pageStartSpinBox->setRange(1, m_document->pages());
    pageStartSpinBox->setEnabled(pageRangeRadioButton->isChecked());
    pageStartSpinBox->setValue(1);
    pageEndSpinBox = new QSpinBox(exportRangeGroupBox);
    pageEndSpinBox->setRange(1, m_document->pages());
    pageEndSpinBox->setEnabled(pageRangeRadioButton->isChecked());
    pageEndSpinBox->setValue(m_document->pages());
    QHBoxLayout *pageRangeLayout = new QHBoxLayout;
    pageRangeLayout->addWidget(pageRangeRadioButton);
    pageRangeLayout->addWidget(pageStartSpinBox);
    pageRangeLayout->addWidget(toLabel);
    pageRangeLayout->addWidget(pageEndSpinBox);
    pageRangeLayout->addStretch();
    QHBoxLayout *customPageRangeLayout = new QHBoxLayout;
    customPageRangeLineEdit = new QLineEdit(exportRangeGroupBox);
    customPageRangeLineEdit->setEnabled(customPageRadioButton->isChecked());
    customPageRangeLayout->addWidget(customPageRadioButton);
    customPageRangeLayout->addWidget(customPageRangeLineEdit);
    
    connect(allPagesRadioButton, &QRadioButton::toggled, this, [=] () {
        pageStartSpinBox->setEnabled(false);
        pageEndSpinBox->setEnabled(false);
        customPageRangeLineEdit->setEnabled(false);
    });
    connect(pageRangeRadioButton, &QRadioButton::toggled, this, [=] () {
        pageStartSpinBox->setEnabled(true);
        pageEndSpinBox->setEnabled(true);
        customPageRangeLineEdit->setEnabled(false);
    });
    connect(customPageRadioButton, &QRadioButton::toggled, this, [=] () {
        pageStartSpinBox->setEnabled(false);
        pageEndSpinBox->setEnabled(false);
        customPageRangeLineEdit->setEnabled(true);
    });
    //// Export Options Layout
    QVBoxLayout *exportRangeLayout = new QVBoxLayout(exportRangeGroupBox);
    exportRangeLayout->addWidget(allPagesRadioButton); 
    exportRangeLayout->addLayout(pageRangeLayout);
    exportRangeLayout->addLayout(customPageRangeLayout); 
    exportRangeLayout->addStretch();

    /// Quality Selection
    defaultQualityRadioButton = new QRadioButton(i18n("Default"), qualitySelectorGroupBox);
    defaultQualityRadioButton->setChecked(true);
    
    customQualityRadioButton = new QRadioButton(i18n("Custom"), qualitySelectorGroupBox);
    
    sliderMin = new QLabel(i18n("0"), qualitySelectorGroupBox);
    sliderMax = new QLabel(i18n("100"), qualitySelectorGroupBox);
    qualitySlider = new QSlider(Qt::Horizontal, qualitySelectorGroupBox);
    qualitySlider->setRange(0, 100);
    qualitySlider->setValue(100);
    qualitySlider->setEnabled(customQualityRadioButton->isChecked());
    qualitySlider->setTickInterval(10);
    qualitySlider->setTickPosition(QSlider::TicksBelow);

    connect(defaultQualityRadioButton, &QRadioButton::toggled, this, [=] () {
        qualitySlider->setEnabled(false);
    });

    connect(customQualityRadioButton, &QRadioButton::toggled, this, [=] () {
        qualitySlider->setEnabled(true);
    });

    //// Quality Selector Layout
    QVBoxLayout *qualitySelectorLayout = new QVBoxLayout(qualitySelectorGroupBox);
    qualitySelectorLayout->addWidget(defaultQualityRadioButton);
    qualitySelectorLayout->addWidget(customQualityRadioButton);
    qualitySelectorLayout->addWidget(qualitySlider);

    //// Quality slider values layout
    QHBoxLayout *sliderValueLayout = new QHBoxLayout;
    sliderValueLayout->addWidget(sliderMin);
    sliderValueLayout->addStretch();
    sliderValueLayout->addWidget(sliderMax);
    qualitySelectorLayout->addLayout(sliderValueLayout);
    qualitySelectorLayout->addStretch();

    /// Group the export options and quality slider layouts
    QHBoxLayout *groupLayout = new QHBoxLayout;
    groupLayout->addWidget(exportRangeGroupBox);
    groupLayout->addWidget(qualitySelectorGroupBox);
    // Export button
    exportButton = new QPushButton(i18n("Export"), this);
    connect(exportButton, &QPushButton::clicked, this, &ExportImageDialog::exportImage);
    cancelButton = new QPushButton(i18n("Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, &ExportImageDialog::cancel);
    defaultButton = new QPushButton(i18n("Default"), this);
    connect(defaultButton, &QPushButton::clicked, this, &ExportImageDialog::setDefaults);

    QHBoxLayout *fileNameLayout = new QHBoxLayout;
    fileNameLayout->addWidget(fileNameLineEdit);
    fileNameLayout->addWidget(fileNameBrowseButton);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(imageTypeLabel, imageTypeComboBox);
    formLayout->addRow(fileNameLabel, fileNameLayout);
    formLayout->addRow(groupLayout);

    // Layout for export and cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(defaultButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);

    // Main Layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addLayout(buttonLayout);

    setLayout(layout);
    setWindowTitle(i18n("Export Image"));
}

void ExportImageDialog::searchFileName()
{
    *m_fileName = QFileDialog::getSaveFileName(this, QString(), QString(), m_filter);
    if (!(m_fileName->isEmpty())) {
        fileNameLineEdit->setText(*m_fileName);
    }
}

void ExportImageDialog::exportImage()
{
    std::cout << "Exported..." << std::endl;
}

void ExportImageDialog::cancel()
{
    QDialog::reject();
}

void ExportImageDialog::setDefaults()
{
    allPagesRadioButton->setChecked(true);
    pageStartSpinBox->setValue(1);
    pageEndSpinBox->setValue(m_document->pages());
    pageStartSpinBox->setEnabled(false);
    pageEndSpinBox->setEnabled(false);
    customPageRangeLineEdit->setText(QStringLiteral(""));
    qualitySlider->setValue(100);
    qualitySlider->setEnabled(false);
    defaultQualityRadioButton->setChecked(true);
}

void ExportImageDialog::notifyPageChanged(int pageNumber, int changedFlags)
{
    return;
}