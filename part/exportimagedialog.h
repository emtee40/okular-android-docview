
#ifndef _OKULAR_EXPORTIMAGEDIALOG_H_
#define _OKULAR_EXPORTIMAGEDIALOG_H_

#include "core/document.h"
#include "core/generator.h"
#include "core/observer.h"

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

class ExportImageDocumentObserver : public Okular::DocumentObserver
{
public:
    void notifyPageChanged(int page, int flags) override;

    Okular::Document *m_document;
    int m_quality;
    QString m_format;
    QString m_dirPath;
};

class ExportImageDialog : public QDialog
{
    Q_OBJECT
public:
    ExportImageDialog(Okular::Document *document, QString *dirPath, QList<Okular::PixmapRequest *> *pixmapRequestList, ExportImageDocumentObserver *observer, int *quality, QString *format, bool *exportCanceled);
    ~ExportImageDialog() override;

private:
    Okular::Document *m_document;
    QString *m_dirPath;
    QList<Okular::PixmapRequest *> *m_pixmapRequestList;
    ExportImageDocumentObserver *m_observer;
    int *m_quality;
    QString *m_format;
    bool *m_exportCanceled;

    QLabel *imageTypeLabel;
    QComboBox *imageTypeComboBox;

    QLabel *dirPathLabel;
    QLineEdit *dirPathLineEdit;

    QGroupBox *exportRangeGroupBox;
    QGroupBox *qualitySelectorGroupBox;
    QRadioButton *allPagesRadioButton;
    QRadioButton *pageRangeRadioButton;
    QRadioButton *customPageRadioButton;
    QSpinBox *pageStartSpinBox;
    QSpinBox *pageEndSpinBox;
    QLabel *toLabel;
    QLineEdit *customPageRangeLineEdit;

    QRadioButton *defaultQualityRadioButton;
    QRadioButton *customQualityRadioButton;
    QSlider *qualitySlider;
    QLabel *sliderMin;
    QLabel *sliderMax;

    QPushButton *exportButton;
    QPushButton *cancelButton;
    QPushButton *defaultButton;
    QPushButton *dirPathBrowseButton;

    void initUI();

private Q_SLOTS:
    void searchFileName();
    void reject() override;
    void exportImage();
    void setDefaults();
}; //

#endif // _OKULAR_EXPORTIMAGEDIALOG_H_