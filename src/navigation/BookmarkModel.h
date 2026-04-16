#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class LVDocView;
class BookmarkRepository;

class BookmarkModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum BookmarkRole {
        XPointerRole  = Qt::UserRole,
        PageRole,
        PercentRole,
        TimestampRole,
    };
    Q_ENUM(BookmarkRole)

    explicit BookmarkModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh(LVDocView *docView,
                 BookmarkRepository *repo,
                 const QString &bookPath);

    void clear();

    int count() const;

    Q_INVOKABLE QString xpointerAt(int index) const;

signals:
    void countChanged();

private:
    struct BookmarkDisplayEntry {
        QString xpointer;
        int     page      = 0;
        int     percent   = 0;
        QString timestamp;
    };

    QVector<BookmarkDisplayEntry> m_entries;
};
