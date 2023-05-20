/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_KJS_FULLSCREEN_P_H
#define OKULAR_SCRIPT_KJS_FULLSCREEN_P_H

#include <QObject>

namespace Okular
{
class JSFullscreen : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loop READ loop WRITE setLoop)
    Q_PROPERTY(bool useTimer READ useTimer WRITE setUseTimer)
    Q_PROPERTY(int timeDelay READ timeDelay WRITE setTimeDelay)

public:
    bool loop() const;
    void setLoop(bool loop);
    bool useTimer() const;
    void setUseTimer(bool use);
    int timeDelay() const;
    void setTimeDelay(int time);
};

}

#endif
