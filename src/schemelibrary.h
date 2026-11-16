// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "scheme.h"

#include <QList>
#include <QString>

/// The schemes available on this machine: the ones shipped with the
/// application, then the ones in the user's data directory. A user scheme
/// replaces a shipped one with the same id.
class SchemeLibrary
{
public:
    /// Reads every scheme file. Broken files are reported and skipped.
    static SchemeLibrary load();

    /// Directory where users can drop their own scheme files.
    static QString userDirectory();
    /// The scheme used when nothing else decides.
    static QString defaultId();

    const QList<Scheme> &all() const { return m_schemes; }
    bool isEmpty() const { return m_schemes.isEmpty(); }

    /// Null if there is no such scheme.
    const Scheme *find(const QString &id) const;
    /// The scheme Fcitx5 calls by this ShuangpinProfile name, or null.
    const Scheme *findByFcitxProfile(const QString &profile) const;
    /// The default scheme, or failing that any scheme; null only when the
    /// library is empty.
    const Scheme *fallback() const;

    void add(const Scheme &scheme);

private:
    QList<Scheme> m_schemes;
};
