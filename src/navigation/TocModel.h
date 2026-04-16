#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class LVDocView;

class TocModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum TocRole {
        TitleRole    = Qt::UserRole,
        LevelRole,
        XPointerRole,
        PageRole,
    };
    Q_ENUM(TocRole)

    explicit TocModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void populate(LVDocView *docView);

    void clear();

    int count() const;

    Q_INVOKABLE QString xpointerAt(int index) const;

    int findChapterForPage(int page0based) const;

signals:
    void countChanged();

private:
    struct TocEntry {
        QString title;
        int     level    = 0;
        int     page0    = 0;
        int     pageDisplay = 1;
        QString xpointer;
    };

    QVector<TocEntry> m_entries;
};
