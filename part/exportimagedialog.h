
#ifndef _OKULAR_EXPORTIMAGEDIALOG_H_
#define _OKULAR_EXPORTIMAGEDIALOG_H_

#include "core/observer.h"
#include "core/document.h"
#include "core/generator.h"

#include <QDialog>
#include <QPixmap>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QPushButton>
#include <QWidget>
#include <QSpinBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QList>

class ExportImageDocumentObserver : public Okular::DocumentObserver
{
public:
    ExportImageDocumentObserver();
    ~ExportImageDocumentObserver();

    void notifyPageChanged(int page, int flags) override;
};

class ExportImageDialog : public QDialog
{
    Q_OBJECT
public:
    ExportImageDialog(QWidget *parent, Okular::Document *document, QString *dirName, QList<Okular::PixmapRequest*> *pixmapRequestList, ExportImageDocumentObserver *observer);
    ~ExportImageDialog() override;

private:

    QWidget *m_parentWidget;
    Okular::Document *m_document;
    QString *m_dirName;
    QList<Okular::PixmapRequest*> *m_pixmapRequestList;
    ExportImageDocumentObserver *m_observer;

    QLabel *imageTypeLabel;
    QComboBox *imageTypeComboBox;

    QLabel *dirNameLabel;
    QLineEdit *dirNameLineEdit;

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
    QPushButton *dirNameBrowseButton;

    void initUI();

    // pixmap request members
    int quality;

private Q_SLOTS:
    void searchFileName();
    void exportImage();
    void reject() override;
    void setDefaults();
}; //

#endif // _OKULAR_EXPORTIMAGEDIALOG_H_