#ifndef QSETTINGS_JSON_H
#define QSETTINGS_JSON_H

// MIT License

// Copyright (c) 2021 MoooDob

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//                                                           copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

//                                       The above copyright notice and this permission notice shall be included in all
//                                       copies or substantial portions of the Software.

//                                          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Pulled from https://github.com/MoooDob/QSettingsJson/
// https://github.com/MoooDob/QSettingsJson/blob/2865a2d4a4bb58ff2612c5f01458c4c0311717ad/LICENSE#L1


// Heavily modified by Khrull

#include <QtCore>
#include <QObject>
#include <QDebug>
class JSONSettingsFormatter
{
public:
    static const QSettings::Format JsonFormat;
private:
    static bool readSettingsJson(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeSettingsJson(QIODevice &device, const QSettings::SettingsMap &map);
    static bool parseJsonValue(const QString &jsonKey, const QJsonValue &jsonValue, QSettings::SettingsMap &map);
    static bool parseJsonArray(const QJsonArray &jsonArray, QSettings::SettingsMap &map);
    static bool parseJsonObject(QJsonObject &jsonObject, QSettings::SettingsMap &map);
};
#endif // QSETTINGS_JSON_H
