#include "exportimagedialog.h"

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>
#include <vector>

#include <KLocalizedString>

#include "core/document.h"
#include "core/observer.h"
#include "core/page.h"

ExportImageDialog::ExportImageDialog(Okular::Document *document, QString *dirPath, QList<Okular::PixmapRequest *> *pixmapRequestList, ExportImageDocumentObserver *observer, QWidget *parent)
    : QDialog(parent)
    , m_document(document)
    , m_dirPath(dirPath)
    , m_pixmapRequestList(pixmapRequestList)
    , m_observer(observer)
{
    initUI();
}

ExportImageDialog::~ExportImageDialog()
{
}

void ExportImageDialog::initUI()
{
    m_imageTypeLabel = new QLabel(i18n("Type:"), this);
    m_PNGTypeLabel = new QLabel(i18n("PNG"), this);

    // Directory Name selection
    m_dirPathLabel = new QLabel(i18n("Output path:"), this);
    m_dirPathLineEdit = new QLineEdit(this);
    m_dirPathLineEdit->setText(QDir::homePath());

    m_dirPathBrowseButton = new QPushButton(i18n("..."), this);
    m_dirPathBrowseButton->setMaximumSize(30, 30);
    connect(m_dirPathBrowseButton, &QPushButton::clicked, this, &ExportImageDialog::searchFileName);

    // Options tab
    m_exportRangeGroupBox = new QGroupBox(i18n("Export range"), this);

    /// Setup Page Export Ranges
    m_allPagesRadioButton = new QRadioButton(i18n("Export all"), m_exportRangeGroupBox);
    m_allPagesRadioButton->setChecked(true);
    m_pageRangeRadioButton = new QRadioButton(i18n("Pages from"), m_exportRangeGroupBox);
    m_toLabel = new QLabel(i18n("to"), m_exportRangeGroupBox);
    m_customPageRadioButton = new QRadioButton(i18n("Pages"), m_exportRangeGroupBox);

    m_pageStartSpinBox = new QSpinBox(m_exportRangeGroupBox);
    m_pageStartSpinBox->setRange(1, m_document->pages());
    m_pageStartSpinBox->setEnabled(m_pageRangeRadioButton->isChecked());
    m_pageStartSpinBox->setValue(1);
    m_pageEndSpinBox = new QSpinBox(m_exportRangeGroupBox);
    m_pageEndSpinBox->setRange(1, m_document->pages());
    m_pageEndSpinBox->setEnabled(m_pageRangeRadioButton->isChecked());
    m_pageEndSpinBox->setValue(m_document->pages());
    QHBoxLayout *pageRangeLayout = new QHBoxLayout;
    pageRangeLayout->addWidget(m_pageRangeRadioButton);
    pageRangeLayout->addWidget(m_pageStartSpinBox);
    pageRangeLayout->addWidget(m_toLabel);
    pageRangeLayout->addWidget(m_pageEndSpinBox);
    pageRangeLayout->addStretch();
    QHBoxLayout *customPageRangeLayout = new QHBoxLayout;
    m_customPageRangeLineEdit = new QLineEdit(m_exportRangeGroupBox);
    m_customPageRangeLineEdit->setEnabled(m_customPageRadioButton->isChecked());
    customPageRangeLayout->addWidget(m_customPageRadioButton);
    customPageRangeLayout->addWidget(m_customPageRangeLineEdit);

    connect(m_allPagesRadioButton, &QRadioButton::toggled, this, [=]() {
        m_pageStartSpinBox->setEnabled(false);
        m_pageEndSpinBox->setEnabled(false);
        m_customPageRangeLineEdit->setEnabled(false);
    });
    connect(m_pageRangeRadioButton, &QRadioButton::toggled, this, [=]() {
        m_pageStartSpinBox->setEnabled(true);
        m_pageEndSpinBox->setEnabled(true);
        m_customPageRangeLineEdit->setEnabled(false);
    });
    connect(m_customPageRadioButton, &QRadioButton::toggled, this, [=]() {
        m_pageStartSpinBox->setEnabled(false);
        m_pageEndSpinBox->setEnabled(false);
        m_customPageRangeLineEdit->setEnabled(true);
    });
    //// Export Options Layout
    QVBoxLayout *exportRangeLayout = new QVBoxLayout(m_exportRangeGroupBox);
    exportRangeLayout->addWidget(m_allPagesRadioButton);
    exportRangeLayout->addLayout(pageRangeLayout);
    exportRangeLayout->addLayout(customPageRangeLayout);
    exportRangeLayout->addStretch();

    /// Group the export options and any other required setting in the future
    QHBoxLayout *groupLayout = new QHBoxLayout;
    groupLayout->addWidget(m_exportRangeGroupBox);
    // Export button
    m_exportButton = new QPushButton(i18n("Export"), this);
    connect(m_exportButton, &QPushButton::clicked, this, &ExportImageDialog::exportImage);
    m_cancelButton = new QPushButton(i18n("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, [this] { QDialog::done(Canceled); });
    m_defaultButton = new QPushButton(i18n("Default"), this);
    connect(m_defaultButton, &QPushButton::clicked, this, &ExportImageDialog::setDefaults);

    QHBoxLayout *dirPathLayout = new QHBoxLayout;
    dirPathLayout->addWidget(m_dirPathLineEdit);
    dirPathLayout->addWidget(m_dirPathBrowseButton);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(m_imageTypeLabel, m_PNGTypeLabel);
    formLayout->addRow(m_dirPathLabel, dirPathLayout);
    formLayout->addRow(groupLayout);

    // Layout for export and cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_defaultButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addWidget(m_cancelButton);

    // Main Layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addLayout(buttonLayout);

    setLayout(layout);
    setWindowTitle(i18n("Export Image"));
}

void ExportImageDialog::searchFileName()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (!(dirPath.isEmpty())) {
        m_dirPathLineEdit->setText(dirPath);
    }
}

void ExportImageDialog::exportImage()
{
    std::vector<std::pair<int, int>> pageRanges;
    if (m_allPagesRadioButton->isChecked()) {
        pageRanges.emplace_back(1, m_document->pages());
    } else if (m_pageRangeRadioButton->isChecked()) {
        int start = m_pageStartSpinBox->value();
        int end = m_pageEndSpinBox->value();
        if (start > end) {
            pageRanges.emplace_back(m_pageStartSpinBox->value(), m_pageStartSpinBox->value());
        } else {
            pageRanges.emplace_back(m_pageStartSpinBox->value(), m_pageEndSpinBox->value());
        }
    } else if (m_customPageRadioButton->isChecked()) {
        QStringList separatePageRanges = m_customPageRangeLineEdit->text().split(QStringLiteral(","), Qt::SkipEmptyParts);
        bool ok;
        for (const QString &part : separatePageRanges) {
            QStringList range = part.split(QStringLiteral("-"));
            if (range.size() == 1) {
                int pageVal = range[0].toInt(&ok);
                if (!ok || pageVal < 1 || pageVal > static_cast<int>(m_document->pages())) {
                    QDialog::done(InvalidOptions);
                    return;
                }
                pageRanges.emplace_back(pageVal, pageVal);
            } else if (range.size() == 2) {
                int pageStart = range[0].toInt(&ok);
                if (!ok || pageStart < 1 || pageStart > static_cast<int>(m_document->pages())) {
                    QDialog::done(InvalidOptions);
                    return;
                }
                int pageEnd = range[1].toInt(&ok);
                if (!ok || pageEnd < 1 || pageEnd > static_cast<int>(m_document->pages())) {
                    QDialog::done(InvalidOptions);
                    return;
                }
                if (pageStart > pageEnd) {
                    pageRanges.emplace_back(pageStart, pageStart);
                } else {
                    pageRanges.emplace_back(pageStart, pageEnd);
                }
            } else {
                QDialog::done(InvalidOptions);
                return;
            }
        }
    }
    for (const std::pair<int, int> &p : pageRanges) {
        for (int i = p.first; i <= p.second; i++) {
            int width = (int)((m_document->page(i - 1))->width());
            int height = (int)((m_document->page(i - 1))->height());
            Okular::PixmapRequest *request = new Okular::PixmapRequest(m_observer, i - 1, width, height, 1 /* dpr */, 1, Okular::PixmapRequest::Asynchronous);
            *m_pixmapRequestList << request;
        }
    }
    *m_dirPath = m_dirPathLineEdit->text();
    QDialog::done(Accepted);
}

void ExportImageDialog::setDefaults()
{
    m_allPagesRadioButton->setChecked(true);
    m_pageStartSpinBox->setValue(1);
    m_pageEndSpinBox->setValue(m_document->pages());
    m_pageStartSpinBox->setEnabled(false);
    m_pageEndSpinBox->setEnabled(false);
    m_customPageRangeLineEdit->setText(QStringLiteral(""));
}

void ExportImageDocumentObserver::notifyPageChanged(int page, int flags)
{
    if (!(flags & Okular::DocumentObserver::Pixmap)) {
        return;
    }
    const QPixmap *pixmap = m_document->page(page)->getPixmap(this);
    QFileInfo info(m_document->documentInfo().get(Okular::DocumentInfo::FilePath));
    QString fileName = info.baseName() + QStringLiteral("_") + QString::number(page + 1) + QStringLiteral(".png");
    QDir dir(m_dirPath);
    QString filePath = dir.filePath(fileName);
    pixmap->save(filePath, "PNG");
}