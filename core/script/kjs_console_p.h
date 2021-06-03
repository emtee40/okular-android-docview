/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_KJS_CONSOLE_P_H
#define OKULAR_SCRIPT_KJS_CONSOLE_P_H

class KJSContext;
class KJSObject;

namespace Okular
{
class JSConsole
{
public:
    static void initType(KJSContext *ctx);
    static KJSObject object(KJSContext *ctx);
};

}

#endif
