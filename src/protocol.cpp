// Lith
// Copyright (C) 2020 Martin Bříza
// Copyright (C) 2020 Václav Kubernát
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; If not, see <http://www.gnu.org/licenses/>.

#include "protocol.h"

#include "lith.h"

#include <QDataStream>
#include <QApplication>
#include <QDateTime>
#include <QAbstractEventDispatcher>

static const QMap<int, QString> weechatColors {
    { 0, "default" },
    { 1, "black" },
    { 2, "dark gray" },
    { 3, "dark red" },
    { 4, "light red" },
    { 5, "dark green" },
    { 6, "light green" },
    { 7, "brown" },
    { 8, "yellow" },
    { 9, "dark blue" },
    { 10, "light blue" },
    { 11, "dark magenta" },
    { 12, "light magenta" },
    { 13, "dark cyan" },
    { 14, "light cyan" },
    { 15, "gray" },
    { 16, "white" }
};


bool Protocol::parse(QDataStream &s, Protocol::Char &r) {
    s.readRawData(&r.d, 1);
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::Integer &r) {
    s.setByteOrder(QDataStream::BigEndian);
    s >> r.d;
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::LongInteger &r) {
    quint8 length;
    s >> length;
    QByteArray buf((int) length + 1, 0);
    s.readRawData(buf.data(), length);
    r.d = buf.toLongLong();
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::String &r, bool canContainHtml) {
    uint32_t len;
    r.d.clear();
    s >> len;
    if (len == uint32_t(-1))
        r.d = QString();
    else if (len == 0)
        r.d = "";
    else if (len > 0) {
        QByteArray buf(len + 1, 0);
        s.readRawData(buf.data(), len);
        r.d = convertColorsToHtml(buf, canContainHtml);
    }
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::Buffer &r) {
    uint32_t len;
    r.d.clear();
    s >> len;
    if (len == 0)
        r.d = "";
    if (len > 0) {
        r.d = QByteArray(len, 0);
        s.readRawData(r.d.data(), len);
    }
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::Pointer &r) {
    quint8 length;;
    s >> length;
    QByteArray buf((int) length + 1, 0);
    s.readRawData(buf.data(), length);
    bool ok = false;
    r.d = buf.toULongLong(&ok, 16);
    if (!ok)
        return false;
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::Time &r) {
    quint8 length;
    s >> length;
    QByteArray buf((int) length + 1, 0);
    s.readRawData(buf.data(), length);
    r.d = buf;
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::HashTable &r) {
    char keyType[4] = { 0 }, valueType[4] = { 0 };
    s.readRawData(keyType, 3);
    if (QString(keyType) != "str") {
        qWarning() << "Hashtable currently supports only string keys";
        return false;
    }
    s.readRawData(valueType, 3);
    if (QString(valueType) != "str") {
        qWarning() << "Hashtable currently supports only string values";
        return false;
    }
    quint32 count = 0;
    s >> count;
    r.d.clear();
    for (quint32 i = 0; i < count; i++) {
        Protocol::String key, value;
        parse(s, key);
        parse(s, value);
        r.d.insert(key.d, value.d);
    }
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::HData &r) {
    Protocol::String hpath;
    Protocol::String keys;
    Protocol::Integer count;
    parse(s, hpath);
    parse(s, keys);
    parse(s, count);
    r.path = hpath.d.split("/");
    r.keys = keys.d.split(",");

    for (int i = 0; i < count.d; i++) {
        Protocol::HData::Item item;
        for (int j = 0; j < r.path.count(); j++) {
            Protocol::Pointer ptr;
            parse(s, ptr);
            item.pointers.append(ptr.d);
        }
        for (int j = 0; j < r.keys.count(); j++) {
            auto name = r.keys[j].split(":").first();
            auto type = r.keys[j].split(":").last();
            if (type == "int") {
                Protocol::Integer i;
                parse(s, i);
                item.objects[name] = QVariant::fromValue(i.d);
            }
            else if (type == "lon") {
                Protocol::LongInteger l;
                parse(s, l);
                item.objects[name] = QVariant::fromValue(l.d);
            }
            else if (type == "str" || type == "buf") {
                Protocol::String str;
                bool canContainHTML = false;
                if (name == "message" || name == "title" || name == "prefix")
                    canContainHTML = true;
                parse(s, str, canContainHTML);
                item.objects[name] = QVariant::fromValue(str.d);
            }
            else if (type == "arr") {
                char fieldType[4] = { 0 };
                s.readRawData(fieldType, 3);
                if (strcmp(fieldType, "int") == 0) {
                    Protocol::ArrayInt a;
                    parse(s, a);
                    item.objects[name] = QVariant::fromValue(a.d);
                }
                else if (strcmp(fieldType, "str") == 0) {
                    Protocol::ArrayStr a;
                    parse(s, a);
                    item.objects[name] = QVariant::fromValue(a.d);
                }
                else {
                    qCritical() << "Unhandled array item type:" << fieldType << "for field" << name;
                }
            }
            else if (type == "tim") {
                Protocol::Time t;
                parse(s, t);
                item.objects[name] = QVariant::fromValue(QDateTime::fromMSecsSinceEpoch(t.d.toLongLong() * 1000));
            }
            else if (type == "ptr") {
                Protocol::Pointer p;
                parse(s, p);
                item.objects[name] = QVariant::fromValue(p.d);
            }
            else if (type == "chr") {
                Protocol::Char c;
                parse(s, c);
                item.objects[name] = QVariant::fromValue(c.d);
            }
            else if (type == "htb") {
                Protocol::HashTable htb;
                parse(s, htb);
                item.objects[name] = QVariant::fromValue(htb.d);
            }
            else {
                qCritical() << "!!! Unhandled type:" << type << "for field" << name;
            }
        }
        r.data.append(item);
    }
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::ArrayInt &r) {
    uint32_t len;
    s >> len;
    for (uint32_t i = 0; i < len; i++) {
        Protocol::Integer num;
        parse(s, num);
        r.d.append(num.d);
    }
    return true;
}

bool Protocol::parse(QDataStream &s, Protocol::ArrayStr &r) {
    uint32_t len;
    s >> len;
    for (uint32_t i = 0; i < len; i++) {
        Protocol::String str;
        parse(s, str);
        r.d.append(str.d);
    }
    return true;
}

QString Protocol::convertColorsToHtml(const QByteArray &data, bool canContainHtml) {
    QString result;
    if (canContainHtml)
        result += "<html><body>";

    bool foreground = false;
    bool background = false;
    bool bold = false;
    bool reverse = false;
    bool italic = false;
    bool underline = false;
    bool keep = false;

    auto endColors = [&result, &foreground, &background]() {
       if (foreground) {
           foreground = false;
           result += "</font>";
       }
       if (background) {
           background = false;
           result += "</span>";
       }
    };
    auto endAttrs = [&result, &bold, &reverse, &italic, &underline, &keep]() {
       if (bold) {
           bold = false;
           result += "</b>";
       }
       if (reverse) {
           reverse = false;
           // TODO
       }
       if (italic) {
           italic = false;
           result += "</i>";
       }
       if (underline) {
           underline = false;
           result += "</u>";
       }
       if (keep) {
           // TODO
       }
    };
    auto loadAttr = [&result, &bold, &reverse, &italic, &underline, &keep](QByteArray::const_iterator &it) {
       while (true) {
           switch(*it) {
           case 0x01: // fallthrough // TODO what the fuck weechat
           case '*':
               if (bold)
                   break;
               bold = true;
               result += "<b>";
               break;
           case '!':
               if (reverse)
                   break;
               reverse = true;
               // TODO
               break;
           case '/':
               if (italic)
                   break;
               italic = true;
               result += "<i>";
               break;
           case '_':
               if (underline)
                   break;
               underline = true;
               result += "<u>";
               break;
           case '|':
               break;
           case '@':
               break;
           case 0x1A:
               break;
           case 0x19:
               break;
           default:
               --it;
               return;
           }
           ++it;
       }
    };

    auto clearAttr = [&result, &bold, &reverse, &italic, &underline, &keep](QByteArray::const_iterator &it) {
       while (true) {
           switch(*it) {
           case 0x01: // fallthrough // TODO what the fuck weechat
           case '*':
               if (bold) {
                   bold = false;
                   result += "</b>";
               }
               break;
           case '!':
               if (reverse) {
                   reverse = false;
                   // TODO
               }
               break;
           case '/':
               if (italic) {
                   italic = false;
                   result += "</i>";
               }
               break;
           case '_':
               if (underline) {
                   underline = false;
                   result += "</u>";
               }
               break;
           case '|':
               break;
           case 0x1B:
               break;
           default:
               --it;
               return;
           }
           ++it;
       }
    };
    auto loadStd = [&result, &foreground](QByteArray::const_iterator &it) {
       while (*it == '@' || *it == '*' || *it == '!' || *it == '/' || *it == '_' || *it == '|')
           ++it;
       int code = 0;
       if (*it == 0x19 || *it == 'F')
           it++;
       for (int i = 0; i < 2; i++) {
           code *= 10;
           code += (*it) - '0';
           ++it;
       }
       --it;
       if (foreground) {
           result += "</font>";
           foreground = false;
       }
       if (weechatColors.contains(code)) {
           result += "<font color=\""+weechatColors[code]+"\">";
           foreground = true;
       }
    };
    auto loadExt = [&result, &foreground](QByteArray::const_iterator &it) {
       while (*it == '@' || *it == '*' || *it == '!' || *it == '/' || *it == '_' || *it == '|')
           ++it;
       int code = 0;
       for (int i = 0; i < 5; i++) {
           code *= 10;
           code += (*it) - '0';
           ++it;
       }
       --it;
       if (foreground) {
           result += "</font>";
           foreground = false;
       }
       result += "<font color=\"green\">";
       foreground = true;
    };
    auto loadBgStd = [&result, &background](QByteArray::const_iterator &it) {
       while (*it == '@' || *it == '*' || *it == '!' || *it == '/' || *it == '_' || *it == '|')
           ++it;
       int code = 0;
       for (int i = 0; i < 2; i++) {
           code *= 10;
           code += (*it) - '0';
           ++it;
       }
       --it;
       if (background)
           result += "</span";
       result += "<span style=\"background-color: blue\">";
       background = true;
    };
    auto loadBgExt = [&result, &background](QByteArray::const_iterator &it) {
       while (*it == '@' || *it == '*' || *it == '!' || *it == '/' || *it == '_' || *it == '|')
           ++it;
       int code = 0;
       for (int i = 0; i < 5; i++) {
           code *= 10;
           code += (*it) - '0';
           ++it;
       }
       --it;
       if (background)
           result += "</span";
       result += "<span style=\"background-color: red\">";
       background = true;
    };
    auto getChar = [](QByteArray::const_iterator &it) -> QString {
       if ((unsigned char) *it < 0x80) {
           return QString(*it);
       }
       else {
           QByteArray buf;
           if ((*it & 0b11111000) == 0b11110000) {
               buf += *it++;
               buf += *it++;
               buf += *it++;
               buf += *it;
           }
           else if ((*it & 0b11110000) == 0b11100000) {
               buf += *it++;
               buf += *it++;
               buf += *it;
           }
           else if ((*it & 0b11100000) == 0b11000000) {
               buf += *it++;
               buf += *it;
           }
           else {
               return QString(*it);
           }
           return QString(buf);
       }
    };
    for (auto it = data.begin(); it != data.end(); ++it) {
       if (*it == 0x19) {
           ++it;
           if (*it == 'F') {
               ++it;
               if (*it == '@') {
                   ++it;
                   loadAttr(it);
                   loadExt(it);
               }
               else {
                   loadAttr(it);
                   loadStd(it);
               }
           }
           else if (*it == 'B') {
               ++it;
               if (*it == '@')
                   loadBgExt(it);
               else
                   loadBgStd(it);
           }
           else if (*it == '*') {
               ++it;
               if (*it == '@') {
                   ++it;
                   loadAttr(it);
                   loadExt(it);
               }
               else {
                   loadAttr(it);
                   loadExt(it);
               }
               if (*it == ',' || *it == '~')
                   ++it;
               if (*it == '@') {
                   ++it;
                   loadAttr(it);
                   loadExt(it);
               }
               else {
                   loadAttr(it);
                   loadExt(it);
               }
           }
           else if (*it == '@') {
               ++it;
               loadExt(it);
           }
           else if (*it == 0x1C) {
               endColors();
           }
           else {
               loadStd(it);
           }
       }
       else if (*it == 0x1C) {
           endColors();
           endAttrs();
       }
       else if (*it == 0x1A) {
           loadAttr(it);
       }
       else if (*it == 0x1B) {
           clearAttr(it);
       }
       else if (canContainHtml && (*it == '<'))
           result += "&lt;";
       else if (canContainHtml && (*it == '>'))
           result += "&gt;";
       else if (canContainHtml && (*it == '&'))
           result += "&amp;";
       else if (canContainHtml && (*it == '"'))
           result += "&quot;";
       else if (canContainHtml && (*it == '\''))
           result += "&apos;";
       else if (*it) {
           result += getChar(it);
       }
    }
    endColors();
    endAttrs();
    if (canContainHtml)
        result += "</body></html>";
    return result;
}

QString Protocol::HData::toString() const {
    QString ret;

    ret += "HDATA\n";
    ret += "- PATH:\n";
    for (auto i : path) {
        ret += "\t" + i + "\n";
    }
    ret += "- KEYS:\n";
    for (auto i : keys) {
        ret += "\t" + i + "\n";
    }
    ret += "-VALUES:\n";
    for (auto i : data) {
        ret += "\t-PATH\n";
        ret += "\t\t";
        for (auto j : i.pointers) {
            ret += QString("%1").arg(j, 8, 16, QChar('0')) + " ";
        }
        ret += "\n";
        ret += "\t-OBJECTS\n";
        for (auto j : i.objects.keys()) {
            ret += QString("\t\t") + j + ": \"" + i.objects[j].toString() + "\"\n";
        }
        ret += "\n";
    }
    return ret;
}
