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
#include <QDir>

#include <vector>
#include <utility>

#include <KLocalizedString>

#include "core/observer.h"
#include "core/document.h"
#include "core/page.h"

ExportImageDialog::ExportImageDialog(QWidget *parent, Okular::Document *document, QString *dirName, QList<Okular::PixmapRequest*> *pixmapRequestList, ExportImageDocumentObserver *observer, int *quality)
    : m_parentWidget(parent)
    , m_document(document)
    , m_dirName(dirName)
    , m_pixmapRequestList(pixmapRequestList)
    , m_observer(observer)
    , m_quality(quality)
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

    // Directory Name selection
    dirNameLabel = new QLabel(i18n("Output path:"), this);
    dirNameLineEdit = new QLineEdit(this);
    *m_dirName = QDir::homePath();
    dirNameLineEdit->setText(*m_dirName);

    dirNameBrowseButton = new QPushButton(i18n("..."), this);
    dirNameBrowseButton->setMaximumSize(30, 30);
    connect(dirNameBrowseButton, &QPushButton::clicked, this, &ExportImageDialog::searchFileName);

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
    connect(exportButton, &QPushButton::clicked, this, &ExportImageDialog::accept);
    cancelButton = new QPushButton(i18n("Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, &ExportImageDialog::reject);
    defaultButton = new QPushButton(i18n("Default"), this);
    connect(defaultButton, &QPushButton::clicked, this, &ExportImageDialog::setDefaults);

    QHBoxLayout *dirNameLayout = new QHBoxLayout;
    dirNameLayout->addWidget(dirNameLineEdit);
    dirNameLayout->addWidget(dirNameBrowseButton);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(imageTypeLabel, imageTypeComboBox);
    formLayout->addRow(dirNameLabel, dirNameLayout);
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
    *m_dirName = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (!(m_dirName->isEmpty())) {
        dirNameLineEdit->setText(*m_dirName);
    }
}

void ExportImageDialog::accept()
{
    std::vector<std::pair<int, int>> pageRanges;
    if(allPagesRadioButton->isChecked())
    {
        pageRanges.push_back({1, m_document->pages()});
    }
    else if(pageRangeRadioButton->isChecked())
    {
        int start = pageStartSpinBox->value();
        int end = pageEndSpinBox->value();
        if(start <= end)
        {
            pageRanges.push_back({pageStartSpinBox->value(), pageEndSpinBox->value()});
        }
        else
        {
            pageRanges.push_back({pageStartSpinBox->value(), pageEndSpinBox->value()});
        }
    }
    else if(customPageRadioButton->isChecked())
    {
        QStringList separatePageRanges = customPageRangeLineEdit->text().split(QStringLiteral(","), Qt::SkipEmptyParts);
        bool ok;
        for (const QString &part : separatePageRanges)
        {
            QStringList range = part.split(QStringLiteral("-"));
            if(range.size() == 1)
            {
                int pageVal = range[0].toInt(&ok);
                if(!ok)
                {
                    reject();
                }
                pageRanges.push_back({pageVal, pageVal});
            }
            else if(range.size() == 2)
            {
                int pageStart = range[0].toInt(&ok);
                int pageEnd = range[1].toInt(&ok);
                if(!ok)
                {
                    reject();
                }
                pageRanges.push_back({pageStart, pageEnd});
            }
            else
            {
                reject();
            }
        }
    }
    *m_quality = defaultQualityRadioButton->isChecked() ? -1 : qualitySlider->value();
    for(const std::pair<int, int> &p : pageRanges)
    {
        for(int i = p.first; i <= p.second; i++)
        {
            int width = (int) ((m_document->page(i-1))->width());
            int height = (int) ((m_document->page(i-1))->height());
            *m_pixmapRequestList << new Okular::PixmapRequest(m_observer, i, width, height,  1 /* dpr */, 1, Okular::PixmapRequest::Asynchronous);
        }
    }
    QDialog::accept();
}

void ExportImageDialog::reject()
{
    *m_dirName = QString();
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

ExportImageDocumentObserver::ExportImageDocumentObserver(int *quality)
{

}

ExportImageDocumentObserver::~ExportImageDocumentObserver()
{

}

void ExportImageDocumentObserver::notifyPageChanged(int page, int flags)
{
    std::cout << page << " " << flags << std::endl;
}