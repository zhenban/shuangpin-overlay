// SPDX-License-Identifier: GPL-3.0-or-later
#include "pinyin.h"

#include <QHash>

namespace {

struct Row {
    const char *initial;
    const char *finals;
};

// g, k and h share one set; so do j, q, x and z, c, s (with small additions).
#define GKH "a e ai ei ao ou an en ang eng ong u ua uo uai ui uan un uang"
#define JQX "i ia ie iao iu ian in iang ing iong u ue uan un"
#define ZCS "a e ai ao ou an en ang eng ong i u uo ui uan un"

const Row kRows[] = {
    {"", "a o e ai ei ao ou an en ang eng er"},
    {"b", "a o ai ei ao an en ang eng i ie iao ian in ing u"},
    {"p", "a o ai ei ao ou an en ang eng i ie iao ian in ing u"},
    {"m", "a o e ai ei ao ou an en ang eng i ie iao iu ian in ing u"},
    {"f", "a o ei ou an en ang eng u"},
    {"d", "a e ai ei ao ou an en ang eng ong i ia ie iao iu ian ing u uo ui uan un"},
    {"t", "a e ai ao ou an ang eng ong i ie iao ian ing u uo ui uan un"},
    {"n", "a e ai ei ao ou an en ang eng ong i ie iao iu ian in iang ing u uo uan ü üe"},
    {"l", "a e ai ei ao ou an ang eng ong i ia ie iao iu ian in iang ing u uo uan un ü üe"},
    {"g", GKH},
    {"k", GKH},
    {"h", GKH},
    {"j", JQX},
    {"q", JQX},
    {"x", JQX},
    {"zh", GKH " i"},
    {"ch", "a e ai ao ou an en ang eng ong i u uo uai ui uan un uang"},
    {"sh", "a e ai ei ao ou an en ang eng i u ua uo uai ui uan un uang"},
    {"r", "e ao ou an en ang eng ong i u uo ui uan un"},
    {"z", ZCS " ei"},
    {"c", ZCS},
    {"s", ZCS},
    {"y", "a e o ao ou an ang ong i in ing u ue uan un"},
    {"w", "a o ai ei an en ang eng u"},
};

struct Table {
    QStringList initials;
    QHash<QString, QStringList> finals;

    Table()
    {
        for (const Row &row : kRows) {
            const QString initial = QString::fromUtf8(row.initial);
            initials << initial;
            finals.insert(initial, QString::fromUtf8(row.finals).split(u' '));
        }
    }
};

const Table &table()
{
    static const Table t;
    return t;
}

} // namespace

namespace Pinyin {

const QStringList &initials()
{
    return table().initials;
}

const QStringList &finalsFor(const QString &initial)
{
    static const QStringList none;
    const auto it = table().finals.constFind(initial);
    return it == table().finals.constEnd() ? none : *it;
}

bool isSyllable(const QString &initial, const QString &final)
{
    return finalsFor(initial).contains(final);
}

} // namespace Pinyin
