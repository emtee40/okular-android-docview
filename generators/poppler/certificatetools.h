/***************************************************************************
 *   Copyright (C) 2012 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _CERTIFICATETOOLS_H_
#define _CERTIFICATETOOLS_H_

#include <QWidget>
class QListWidget;
class QPushButton;

class CertificateTools : public QWidget
{
    Q_OBJECT

public:
    explicit CertificateTools(QWidget *parent = nullptr);
    ~CertificateTools() override;

protected:
    QListWidget *m_list;
};

#endif
