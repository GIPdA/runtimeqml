#include "runtimeqml.hpp"

#include <QFileInfo>
#include <QFileSelector>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QQmlAbstractUrlInterceptor>
#include <QRegularExpression>
#include <QTimer>
#include <QQuickWindow>
#include <QXmlStreamReader>

#include <map>

static Q_LOGGING_CATEGORY(log, "RuntimeQml")

struct DeferTarget {};
template <class Lambda>
struct Defer {
    ~Defer() { lambda(); }
    Lambda lambda;
};

template <class Lambda>
Defer<Lambda> operator << (DeferTarget, Lambda lambda) {
    return { std::move(lambda) };
}

#define defer auto defer_ = DeferTarget() << [&]()


// From QRegularExpression, but matching path separators too.
static QString wildcardToRegularExpression(QStringView pattern)
{
    const int wclen = pattern.length();
    QString rx;
    rx.reserve(wclen + wclen / 16);
    int i = 0;
    const QChar *wc = pattern.data();

#ifdef Q_OS_WIN
    //const QLatin1Char nativePathSeparator('\\');
    const QLatin1String starEscape(".*");
    const QLatin1String questionMarkEscape(".");
#else
    //const QLatin1Char nativePathSeparator('/');
    const QLatin1String starEscape(".*");
    const QLatin1String questionMarkEscape(".");
#endif

    while (i < wclen) {
        const QChar c = wc[i++];
        switch (c.unicode()) {
        case '*':
            rx += starEscape;
            break;
        case '?':
            rx += questionMarkEscape;
            break;
        case '\\':
#ifdef Q_OS_WIN
        case '/':
            rx += QLatin1String("[/\\\\]");
            break;
#endif
        case '$':
        case '(':
        case ')':
        case '+':
        case '.':
        case '^':
        case '{':
        case '|':
        case '}':
            rx += QLatin1Char('\\');
            rx += c;
            break;
        case '[':
            rx += c;
            // Support for the [!abc] or [!a-c] syntax
            if (i < wclen) {
                if (wc[i] == QLatin1Char('!')) {
                    rx += QLatin1Char('^');
                    ++i;
                }

                if (i < wclen && wc[i] == QLatin1Char(']'))
                    rx += wc[i++];

                while (i < wclen && wc[i] != QLatin1Char(']')) {
                    // The '/' appearing in a character class invalidates the
                    // regular expression parsing. It also concerns '\\' on
                    // Windows OS types.
                    //if (wc[i] == QLatin1Char('/') || wc[i] == nativePathSeparator)
                      //  return rx;
                    if (wc[i] == QLatin1Char('\\'))
                        rx += QLatin1Char('\\');
                    rx += wc[i++];
                }
            }
            break;
        default:
            rx += c;
            break;
        }
    }

    return rx;// anchoredPattern(rx);
}


class UrlInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    UrlInterceptor() = default;
    Q_DISABLE_COPY_MOVE(UrlInterceptor)

    QUrl intercept(QUrl const& url, QQmlAbstractUrlInterceptor::DataType /*type*/) final
    {
        /*QString const url_str = url.toString();
        if ( ! url_str.startsWith(QStringLiteral("qrc")) || url_str.endsWith(QStringLiteral("qmldir")))
            return url;
        qCDebug(log, "Intercepting: %s", qPrintable(url_str));//*/

        if (filesMap.count(url) > 0) {
            //qCDebug(log, "Intercepted: %s to %s", qPrintable(url.toString()), qPrintable(filesMap[url].toString()));
            return filesMap[url];
        }
        return url;
    }

    //! Intercepts 'source' to be replaced by 'target'
    void addUrlMap(QUrl const& source, QUrl const& target) {
        //qCDebug(log, "Intercepting: %s to %s", qPrintable(source.toString()), qPrintable(target.toString()));
        filesMap[source] = target;
    }

private:
    std::map<QUrl,QUrl> filesMap;
};


class RuntimeQmlPrivate
{
    Q_DISABLE_COPY(RuntimeQmlPrivate)
    Q_DECLARE_PUBLIC(RuntimeQml)
    RuntimeQml * const q_ptr {nullptr};

    RuntimeQmlPrivate(RuntimeQml* q, QQmlApplicationEngine* _engine) :
        q_ptr(q),
        engine(_engine)
    {
        // Reload timer to combine mutiple quasi-simultaneous file changes.
        reloader.setInterval(200);
        reloader.setSingleShot(true);
        reloader.callOnTimeout(q, [this](){ reloadQml(); });

        // The URL interceptor is used to replace QRC files by filesystem files.
        engine->addUrlInterceptor(&urlInterceptor);

        QObject::connect(&fileWatcher, &QFileSystemWatcher::fileChanged, q_ptr, [&](QString const& path) {
            if ( ! autoReload)
                return;

            qCDebug(log) << "File changed:" << path;
            q_ptr->reload();

#if defined(Q_OS_WIN) || (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
            // Deleted files are removed from the watcher, re-add the file for
            //  systems that delete files to update them.
            // Seems to happen on Windows and Linux.
            QTimer::singleShot(500, &fileWatcher, [this,path](){
                 fileWatcher.addPath(path);
            });
#endif
        });
    }

    void loadQrc(QString const& qrcFilename)
    //! Process QRC entries: add filtered files to watcher and URL interceptor
    {
        QFile file(qrcFilename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(log, "Unable to open resource file '%s'. Error: %s",
                      qPrintable(qrcFilename), qPrintable(file.errorString()));
            return;
        }

        QString const basePath = QFileInfo(qrcFilename).absolutePath() + "/";
        QString currentPrefix;

        QXmlStreamReader inputStream(&file);
        while (!inputStream.atEnd() && !inputStream.hasError()) {
            inputStream.readNext();
            if (inputStream.isStartElement()) {
                QString name { inputStream.name().toString() };

                // Check prefix
                if (name == QStringLiteral("qresource")) {
                    if (inputStream.attributes().hasAttribute(QStringLiteral("prefix"))) {
                        currentPrefix = inputStream.attributes().value(QStringLiteral("prefix")).toString();
                        if ( ! currentPrefix.endsWith('/'))
                            currentPrefix += '/';
                    }
                }

                // Check file name
                else if (name == QStringLiteral("file") && ! currentPrefix.isEmpty()) {
                    // If an alias is set, use it as the resource name.
                    // Filename may either be relative to the QRC file, or absolute (starts with '/').
                    QString const alias { inputStream.attributes().value(QStringLiteral("alias")).toString() };
                    QString const filename { inputStream.readElementText() };
                    QFileInfo const fileInfo { filename };

                    QString const resourceName = alias.isEmpty() ? filename : alias;

                    if ( ! allowedSuffixList.contains(fileInfo.suffix()))
                        continue;

                    // Check ignore list
                    QString const fullQrcFilename = selector.select( [&]() {
                        if (currentPrefix == '/')
                            return "qrc:/" + resourceName;
                        return "qrc:" + currentPrefix + resourceName;
                    }() );

                    bool const toIgnore = std::find_if(fileIgnoreList.cbegin(), fileIgnoreList.cend(), [&](QString const& pattern) {
                        QRegularExpression re(wildcardToRegularExpression(pattern));
                        return re.match(fullQrcFilename).hasMatch();
                    }) != fileIgnoreList.cend();

                    // Add local file name to watcher, and map with QRC file name.
                    QFileInfo const fi(filename);
                    QString const localFilename { selector.select( fi.isAbsolute() ? filename : (basePath + filename) ) };
                    urlInterceptor.addUrlMap(fullQrcFilename, QUrl::fromLocalFile(localFilename));

                    //qCDebug(log, "File to watch: %s - as %s", qPrintable(localFilename), qPrintable(fullQrcFilename));

                    if ( ! toIgnore)
                        fileWatcher.addPath(localFilename);
                }
            }
        }

        { // Debug
            qsizetype const fileCount = fileWatcher.files().size();

            if (fileCount == 0) {
                qCDebug(log, "No files to watch from '%s'.", qPrintable(qrcFilename));
            } else {
                qCDebug(log, "Watching QML files from '%s':", qPrintable(qrcFilename));

                for (auto &f : fileWatcher.files()) {
                    if (f.startsWith(basePath))
                        qCDebug(log) << "  " << f.mid(basePath.length());
                }
            }

            //qCDebug(log, "Total watched files across QRCs: %lld", fileCount);
        }
    }


    void reloadQml()
    //! Close all windows, delete root item and load the main URL again.
    {
        reloading = true;
        defer {
            reloading = false;
        };

        if (mainQmlFile.isEmpty()) {
            qCWarning(log, "Can't reload: no file specified.");
            return;
        }

        qCDebug(log, "Reloading.");

        QQuickWindow* rootWindow = [&]() -> QQuickWindow* {
            auto rootObjects { engine->rootObjects() };
            if (!rootObjects.isEmpty())
                return qobject_cast<QQuickWindow*>(rootObjects.back());
            return nullptr;
        }();

        if (rootWindow) {
            // Find all child windows and close them
            QList<QQuickWindow*> const allWindows = rootWindow->findChildren<QQuickWindow*>();
            for (QQuickWindow* w : allWindows) {
                if (w)
                    w->close();
            }

            rootWindow->close();
            delete rootWindow; // Hopefully all goes well...
            rootWindow = nullptr;
            //rootWindow->deleteLater(); // FIXME ? Causes type errors on Qt 5.13+
        }

        engine->clearComponentCache();
        // TODO: test with network files
        // TODO: QString path to QUrl doesn't work under Windows with load() (load fail)
        engine->load(mainQmlFile);
        //engine->load(m_selector.select(qrcAbsolutePath() + "/" + m_mainQmlFilename));
        // NOTE: QQmlApplicationEngine::rootObjects() isn't cleared, should it be?

        if (engine->rootObjects().isEmpty()) {
            qCWarning(log, "Reloading may have failed! (no root object)");
        }

        emit q_ptr->reloaded();
    }


    QPointer<QQmlApplicationEngine> engine;
    QFileSystemWatcher fileWatcher;
    UrlInterceptor urlInterceptor;
    QFileSelector selector;
    QTimer reloader;

    QUrl mainQmlFile; // File to reload.

    QList<QString> fileIgnoreList;
    QList<QString> allowedSuffixList { "qml" };

    bool autoReload {false};
    bool reloading {false};
};



RuntimeQml::RuntimeQml(QQmlApplicationEngine* engine, QObject* parent) :
    QObject(parent),
    dd_ptr(new RuntimeQmlPrivate(this, engine))
//! Ownership of @a engine is left to the caller.
{}

RuntimeQml::~RuntimeQml()
{}


void RuntimeQml::parseQrc(QString const& qrcFilename)
{
    Q_D(RuntimeQml);
    d->loadQrc(qrcFilename);
}

void RuntimeQml::load(QUrl const& url)
//! Load the URL in the engine, and in the upcoming reloads.
//! Call instead of engine.load(...), or use @a setReloadUrl.
{
    Q_D(RuntimeQml);
    setReloadUrl(url);
    d->engine->load(url);
}

void RuntimeQml::setReloadUrl(QUrl const& url)
//! Sets the URL to load in the upcoming reloads.
{
    Q_D(RuntimeQml);
    d->mainQmlFile = url;
}

QUrl const& RuntimeQml::reloadUrl() const
{
    Q_D(const RuntimeQml);
    return d->mainQmlFile;
}

void RuntimeQml::reload()
//! Trigger a reload on the next event loop.
//! On reload, all windows are closed and the root object is deleted.
{
    Q_D(RuntimeQml);
    // The timer avoids multiple reloads when saving multiple files at once
    if ( ! d->reloader.isActive())
        d->reloader.start();
    //QMetaObject::invokeMethod(this, "reloadQml", Qt::QueuedConnection);
}

bool RuntimeQml::isReloading() const
{
    Q_D(const RuntimeQml);
    return d->reloading;
}


bool RuntimeQml::autoReload() const
//! @property autoReload
//! @brief If true, a reload is triggered when a file is modified on disk.
//! Otherwise, you need to trigger a reload manually from your code by calling reload().
//! @sa reload
{
    Q_D(const RuntimeQml);
    return d->autoReload;
}

void RuntimeQml::setAutoReload(bool autoReload)
{
    Q_D(RuntimeQml);
    if (d->autoReload == autoReload)
        return;

    d->autoReload = autoReload;
    emit autoReloadChanged(autoReload);
}


void RuntimeQml::addFileSuffixFilter(QString const& suffix)
//! @brief Allow a file suffix to be considered in QRC files (e.g. "qml" for *.qml files).
//! Must be called before load() and parseQrc().
//! By default only "qml" files are considered.
{
    Q_D(RuntimeQml);
    if (d->allowedSuffixList.contains(suffix))
        return;

    d->allowedSuffixList.append(suffix);
}

QList<QString> const& RuntimeQml::allowedSuffixes() const
{
    Q_D(const RuntimeQml);
    return d->allowedSuffixList;
}


void RuntimeQml::addIgnoreFilter(QString const& filter)
//! @brief Add a filter to ignore files. Supports "file globbing" matching using wildcards.
//! Applies to the full prefix+filename in the QRC entry (i.e. the QRC path, not the local filesystem path).
//! Must be called before load() and parseQrc().
{
    Q_D(RuntimeQml);
    if (d->fileIgnoreList.contains(filter))
        return;

    d->fileIgnoreList.append(filter);
}


void RuntimeQml::addQrcPrefixIgnoreFilter(QString const& prefix)
//! @brief Add a QRC prefix to ignore. Equivalent to addIgnoreFilter("qrc:/<prefix>") for a valid prefix.
//! Must be called before load() and parseQrc().
{
    if ( (! prefix.isEmpty()) && (! prefix.startsWith(QStringLiteral("qrc:/"))) && (prefix != QStringLiteral("/")))
        addIgnoreFilter("qrc:/"+prefix);
}

QList<QString> const& RuntimeQml::ignoreFiltersList() const
{
    Q_D(const RuntimeQml);
    return d->fileIgnoreList;
}
