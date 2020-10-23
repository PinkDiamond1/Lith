#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "common.h"

#include <QObject>
#include <QDateTime>
#include <QAbstractListModel>
#include <QSet>

class Buffer;
class BufferLine;
class LineModel;

#include <cstdint>

typedef uint64_t pointer_t;

namespace W {
    struct Char { char d { 0 }; };
    struct Integer { int32_t d { 0 }; };
    struct LongInteger { int64_t d { 0 }; };
    struct String { QString d {}; };
    struct Buffer { QByteArray d {}; };
    struct Pointer { pointer_t d { 0 }; };
    struct Time { QString d {}; };
    struct HashTable { QMap<QString, QString> d {}; };
    struct HData { };
    struct ArrayInt { QList<int> d {}; };
    struct ArrayStr { QStringList d {}; };

    QDataStream &operator>>(QDataStream &s, Char &r);
    QDataStream &operator>>(QDataStream &s, Integer &r);
    QDataStream &operator>>(QDataStream &s, LongInteger &r);
    QDataStream &operator>>(QDataStream &s, String &r);
    QDataStream &operator>>(QDataStream &s, Buffer &r);
    QDataStream &operator>>(QDataStream &s, Pointer &r);
    QDataStream &operator>>(QDataStream &s, Time &r);
    QDataStream &operator>>(QDataStream &s, HashTable &r);
    QDataStream &operator>>(QDataStream &s, HData &r);
    QDataStream &operator>>(QDataStream &s, ArrayInt &r);
    QDataStream &operator>>(QDataStream &s, ArrayStr &r);
};


class Nick : public QObject {
    Q_OBJECT
    PROPERTY(char, visible)
    PROPERTY(char, group)
    PROPERTY(int, level)
    PROPERTY(QString, name)
    PROPERTY(QString, color)
    PROPERTY(QString, prefix)
    PROPERTY(QString, prefix_color)

    PROPERTY(pointer_t, ptr)
public:
    Nick(Buffer *parent = nullptr);

};

class Buffer : public QObject {
    Q_OBJECT
    PROPERTY(int, number)
    PROPERTY(QString, name)
    ALIAS(QString, name, full_name)
    PROPERTY(QString, title)
    PROPERTY(QStringList, local_variables)

    PROPERTY(int, unreadMessages)
    PROPERTY(int, hotMessages)

    Q_PROPERTY(LineModel *lines READ lines CONSTANT)
    Q_PROPERTY(QList<QObject*> nicks READ nicks NOTIFY nicksChanged)
public:
    Buffer(QObject *parent, pointer_t pointer);
    //BufferLine *getLine(pointer_t ptr);
    void appendLine(BufferLine *line);

    bool isAfterInitialFetch();

    LineModel *lines();
    QList<QObject*> nicks();
    Q_INVOKABLE Nick *getNick(pointer_t ptr);

public slots:
    void input(const QString &data);
    void fetchMoreLines();

signals:
    void nicksChanged();

private:
    LineModel *m_lines { nullptr };
    QList<QObject*> m_nicks {};
    pointer_t m_ptr;
    bool m_afterInitialFetch { false };
    int m_lastRequestedCount { 0 };
};

class BufferLineSegment : public QObject {
    Q_OBJECT
public:
    enum Type {
        PLAIN,
        LINK,
        EMBED,
        IMAGE,
        VIDEO
    }; Q_ENUMS(Type)
    PROPERTY(Type, type)
    PROPERTY(QString, plainText)
    PROPERTY(QString, summary)
    PROPERTY(QString, embedUrl)
public:
    BufferLineSegment(BufferLine *parent = nullptr, const QString &text = QString(), Type type = PLAIN);

};

class BufferLine : public QObject {
    Q_OBJECT
    PROPERTY(QDateTime, date)
    PROPERTY(bool, displayed)
    PROPERTY(bool, highlight)
    PROPERTY(QStringList, tags_array)
    PROPERTY(QString, prefix)
    PROPERTY(QString, message)

    Q_PROPERTY(bool isPrivMsg READ isPrivMsg NOTIFY tags_arrayChanged)
    Q_PROPERTY(QObject *buffer READ bufferGet WRITE bufferSet NOTIFY bufferChanged)
    Q_PROPERTY(QList<QObject*> segments READ segments NOTIFY segmentsChanged)
public:
    BufferLine(QObject *parent);

    bool isPrivMsg();

    QObject *bufferGet();
    void bufferSet(QObject *o);

    QList<QObject*> segments();

signals:
    void bufferChanged();
    void segmentsChanged();

private slots:
    void onMessageChanged();

private:
    QList<QObject*> m_segments;
};

class LineModel : public QAbstractListModel {
    Q_OBJECT
public:
    LineModel(Buffer *parent);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void appendLine(BufferLine *line);
private:
    QList<BufferLine*> m_lines;
    QSet<pointer_t> m_ptrs;
};

class HotListItem : public QObject {
    Q_OBJECT
    PROPERTY(QList<int>, count)
    PROPERTY_NOSETTER(Buffer*, buffer)
public:
    HotListItem(QObject *parent = nullptr);

    void bufferSet(Buffer *o);

private slots:
    void onCountChanged();
};
#endif // DATAMODEL_H
