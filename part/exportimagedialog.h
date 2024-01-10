
#ifndef _OKULAR_EXPORTIMAGEDIALOG_H_
#define _OKULAR_EXPORTIMAGEDIALOG_H_

#include "core/document.h"
#include "core/generator.h"
#include "core/observer.h"

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
    void getPixmapAndSave(int page);
    void addToPixmapRequestList(Okular::PixmapRequest *request);
    bool getOrRequestPixmaps();

    Okular::Document *m_document;
    QString m_dirPath;
    QList<Okular::PixmapRequest *> m_pixmapRequestList;
};

class ExportImageDialog : public QDialog
{
    Q_OBJECT
public:
    enum DialogCloseCode { Accepted, Canceled, InvalidOptions };

    ExportImageDialog(Okular::Document *document, QString *dirPath, ExportImageDocumentObserver *observer, QWidget *parent = nullptr);
    ~ExportImageDialog() override;

private:
    Okular::Document *m_document;
    QString *m_dirPath;
    ExportImageDocumentObserver *m_observer;

    QLabel *m_imageTypeLabel;
    QLabel *m_PNGTypeLabel;

    QLabel *m_dirPathLabel;
    QLineEdit *m_dirPathLineEdit;

    QGroupBox *m_exportRangeGroupBox;
    QRadioButton *m_allPagesRadioButton;
    QRadioButton *m_pageRangeRadioButton;
    QRadioButton *m_customPageRadioButton;
    QSpinBox *m_pageStartSpinBox;
    QSpinBox *m_pageEndSpinBox;
    QLabel *m_toLabel;
    QLineEdit *m_customPageRangeLineEdit;

    QPushButton *m_dirPathBrowseButton;

    void initUI();

private Q_SLOTS:
    void searchFileName();
    void exportImage();
    void setDefaults();
}; //

#endif // _OKULAR_EXPORTIMAGEDIALOG_H_