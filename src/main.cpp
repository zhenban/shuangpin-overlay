// SPDX-License-Identifier: GPL-3.0-or-later
#include "chartpainter.h"
#include "config.h"
#include "overlaywindow.h"
#include "schemelibrary.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QLocale>
#include <QLockFile>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QTextStream>
#include <QTranslator>

namespace {

constexpr qreal kExportScale = 2.0;
constexpr QChar kExportBackspace = u'<';

int listSchemes(const SchemeLibrary &schemes)
{
    QTextStream out(stdout);
    for (const Scheme &scheme : schemes.all())
        out << scheme.id() << '\t' << scheme.displayName() << '\n';
    return 0;
}

/// Writes a picture of the chart as it looks after @p typed: scheme keys,
/// with '<' standing for Backspace. Keys in @p pressed are drawn held down.
int exportChart(const SchemeLibrary &schemes, const QString &schemeId, const QString &typed, const QString &pressed,
                const QString &file)
{
    QTextStream err(stderr);
    const Scheme *scheme = schemes.find(schemeId);
    if (!scheme) {
        err << QCoreApplication::translate("main", "Unknown scheme: %1").arg(schemeId) << '\n';
        return 1;
    }
    Composition composition;
    composition.setScheme(*scheme);
    for (const QChar typedKey : typed.toLower()) {
        if (typedKey == kExportBackspace) {
            composition.backspace(0);
        } else if (scheme->isKey(typedKey)) {
            composition.key(typedKey, 0);
        } else {
            err << QCoreApplication::translate("main", "Not a key of this scheme: %1").arg(typedKey) << '\n';
            return 1;
        }
    }
    ChartState state = ChartState::of(composition, true);
    for (const QChar pressedKey : pressed.toLower())
        state.lit.insert(pressedKey, 1.0);

    QPixmap pixmap(ChartPainter::size(*scheme, kExportScale));
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        ChartPainter::paint(painter, *scheme, state, ChartTheme(), kExportScale);
    }
    if (!pixmap.save(file)) {
        err << QCoreApplication::translate("main", "Could not write %1").arg(file) << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(APP_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral(APP_ID));
    QGuiApplication::setWindowIcon(
        QIcon::fromTheme(QStringLiteral(APP_ID), QIcon(QStringLiteral(":/icons/app.svg"))));

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral(APP_NAME), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QCoreApplication::installTranslator(&translator);
    QGuiApplication::setApplicationDisplayName(QCoreApplication::translate("main", "Shuangpin Overlay"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "A floating Shuangpin key chart that follows what you type."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption listOption(QStringLiteral("list-schemes"),
                                        QCoreApplication::translate("main", "List the available schemes and exit."));
    const QCommandLineOption exportOption(QStringLiteral("export"),
                                          QCoreApplication::translate("main", "Save the chart as an image and exit."),
                                          QCoreApplication::translate("main", "file"));
    const QCommandLineOption schemeOption(QStringLiteral("scheme"),
                                          QCoreApplication::translate("main", "Scheme to export."),
                                          QCoreApplication::translate("main", "id"), SchemeLibrary::defaultId());
    const QCommandLineOption afterOption(
        QStringLiteral("after"),
        QCoreApplication::translate("main", "Export the chart as it looks after typing these keys; < is Backspace."),
        QCoreApplication::translate("main", "keys"));
    const QCommandLineOption pressedOption(
        QStringLiteral("pressed"), QCoreApplication::translate("main", "Export the chart with these keys held down."),
        QCoreApplication::translate("main", "keys"));
    parser.addOptions({listOption, exportOption, schemeOption, afterOption, pressedOption});
    parser.process(app);

    const SchemeLibrary schemes = SchemeLibrary::load();
    if (parser.isSet(listOption))
        return listSchemes(schemes);
    if (parser.isSet(exportOption))
        return exportChart(schemes, parser.value(schemeOption), parser.value(afterOption), parser.value(pressedOption),
                           parser.value(exportOption));

    // One overlay per session is enough.
    QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtime.isEmpty())
        runtime = QDir::tempPath();
    QLockFile lock(runtime + QStringLiteral("/" APP_NAME ".lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock()) {
        QTextStream(stderr) << QCoreApplication::translate("main", "%1 is already running.")
                                   .arg(QGuiApplication::applicationDisplayName())
                            << '\n';
        return 0;
    }

    OverlayWindow window(schemes);
    window.show();
    return app.exec();
}
