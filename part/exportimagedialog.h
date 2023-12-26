
#ifndef _OKULAR_EXPORTIMAGEDIALOG_H_
#define _OKULAR_EXPORTIMAGEDIALOG_H_

#include "core/observer.h"
#include "core/document.h"

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

class ExportImageDialog : public QDialog, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    ExportImageDialog(QWidget *parent, Okular::Document *m_document, QString *fileName, QString filter);
    ~ExportImageDialog() override;

    void notifyPageChanged(int pageNumber, int changedFlags) override;

private:

    QWidget *m_parentWidget;
    Okular::Document *m_document;

    QLabel *imageTypeLabel;
    QComboBox *imageTypeComboBox;

    QLabel *fileNameLabel;
    QLineEdit *fileNameLineEdit;

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
    QPushButton *fileNameBrowseButton;

    QString *m_fileName;
    QString m_filter;
    void initUI();

    // pixmap request members
    int quality;

private Q_SLOTS:
    void searchFileName();
    void exportImage();
    void cancel();
    void setDefaults();
}; //

#endif // _OKULAR_EXPORTIMAGEDIALOG_H_