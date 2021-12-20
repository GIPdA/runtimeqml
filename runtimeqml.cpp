#include "runtimeqml.h"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTimer>
#include <QXmlStreamReader>

static Q_LOGGING_CATEGORY(log, "RuntimeQML")

/*!
 * \brief Construct a RuntimeQML object with a path to the qrc file.
 * \param engine App engine to reload.
 * \param qrcFilename File name of the QRC file for auto reload.
 * \param parent Pointer to a parent object.
 */
RuntimeQML::RuntimeQML(QQmlApplicationEngine* engine, const QString &qrcFilename, QObject *parent) :
    QObject(parent),
    m_engine(engine),
    m_qrcFilename(qrcFilename),
    m_mainQmlFilename("main.qml")
{
    m_allowedSuffixList << "qml";
}


/*!
 * \brief Returns the absolute path for the given qml file.
 * \param qmlFile Qml filename
 */
QString RuntimeQML::adjustPath(QString qmlFile)
{
    return m_selector.select(qrcAbsolutePath() + "/" + qmlFile);
}

/*!
 * \brief Returns the absolute path to the QRC file.
 */
QString RuntimeQML::qrcAbsolutePath() const
{
    return QFileInfo(m_qrcFilename).absolutePath();
}

/*!
 * \brief Filename of the QRC file.
 */
QString RuntimeQML::qrcFilename() const
{
    return m_qrcFilename;
}

/*!
 * \brief If true, files are watched for changes and auto-reloaded.
 * Otherwise, you need to trigger a reload manually from your code by calling reload().
 * \sa reload
 */
bool RuntimeQML::autoReload() const
{
    return m_autoReload;
}

/*!
 * \brief If true, all open windows will be closed upon reload.
 * \default true
 */
bool RuntimeQML::closeAllOnReload() const
{
    return m_closeAllOnReload;
}

/*!
 * \brief QRC prefixes that are ignored.
 */
const QList<QString>& RuntimeQML::prefixIgnoreList() const
{
    return m_prefixIgnoreList;
}

/*!
 * \brief Files that are ignored.
 */
const QList<QString> &RuntimeQML::fileIgnoreList() const
{
    return m_fileIgnoreList;
}

/*!
 * \brief Allowed suffixes to filter files to watch for changes.
 * By default contains only "qml".
 */
const QList<QString> &RuntimeQML::allowedSuffixes() const
{
    return m_allowedSuffixList;
}

/*!
 * \brief Call it if you don't want debug outputs from this class.
 */
void RuntimeQML::noDebug()
{
    if (m_noDebug)
        return;
    m_noDebug = true;
}

/*!
 * \brief First load of the main QML file.
 * Only call once, in your main() in place of the usual 'engine.load(url);'.
 * Do not use @a reload(), it can lead to some issues with write-once properties.
 */
void RuntimeQML::load()
{
    m_engine->load(m_selector.select(qrcAbsolutePath() + "/" + m_mainQmlFilename));
}

/*!
 * \brief Reload the window.
 */
void RuntimeQML::reload()
{
    QMetaObject::invokeMethod(this, "reloadQml", Qt::QueuedConnection);
}


/*!
 * \brief Call it from QML to set the current QQuickWindow.
 * You shouldn't need to call it as it is done automatically on reload.
 * \param window
 */
void RuntimeQML::setWindow(QQuickWindow* window)
{
    if (window == m_window)
        return;
    m_window = window;
}


/*!
 * \brief Set the QRC filename for files to watch for changes.
 * \param qrcFilename Path to a .qrc file.
 */
void RuntimeQML::setQrcFilename(QString qrcFilename)
{
    if (m_qrcFilename == qrcFilename)
        return;

    m_qrcFilename = qrcFilename;
    emit qrcFilenameChanged(qrcFilename);

    loadQrcFiles();
}

/*!
 * \brief Set the name of the main qml file.
 * Default is "main.qml".
 * \param filename The main qml filename.
 */
void RuntimeQML::setMainQmlFilename(QString filename)
{
    if (m_mainQmlFilename == filename)
        return;

    m_mainQmlFilename = filename;
}


/*!
 * \brief If true, files are watched for changes and auto-reloaded.
 * Otherwise, you need to trigger a reload manually from your code by calling reload().
 * \param reload True to auto-reload, false otherwise.
 */
void RuntimeQML::setAutoReload(bool autoReload)
{
    if (m_autoReload == autoReload)
        return;

    m_autoReload = autoReload;
    emit autoReloadChanged(autoReload);

    if (autoReload)
        loadQrcFiles();
    else
        unloadFileWatcher();
}

/*!
 * \brief If true, all open windows are closed upon reload. Otherwise, might cause "link" errors with QML components.
 * \param closeAllOnReload True to close all windows on reload, false otherwise.
 */
void RuntimeQML::setCloseAllOnReload(bool closeAllOnReload)
{
    if (m_closeAllOnReload == closeAllOnReload)
        return;

    m_closeAllOnReload = closeAllOnReload;
    emit closeAllOnReloadChanged(m_closeAllOnReload);
}

/*!
 * \brief Add a QRC prefix to ignore.
 * \note Relevant for auto-reload only.
 * \param prefix Prefix to ignore.
 */
void RuntimeQML::ignoreQrcPrefix(const QString& prefix)
{
    if (m_prefixIgnoreList.contains(prefix))
        return;

    m_prefixIgnoreList.append(prefix);

    if (m_autoReload)
        loadQrcFiles();
}

/*!
 * \brief Add a filename to ignore from changes.
 * Applies to the full filename in the QRC entry (i.e. the local "path"), with the prefix.
 * Supports "file globbing" matching using wildcards.
 * \note Relevant for auto-reload only.
 * \param filename Filename to ignore.
 */
void RuntimeQML::ignoreFile(const QString &filename)
{
    if (m_fileIgnoreList.contains(filename))
        return;

    m_fileIgnoreList.append(filename);

    if (m_autoReload)
        loadQrcFiles();
}

/*!
 * \brief Allow a file suffix to be watched for changes.
 * \note Relevant for auto-reload only.
 * \param suffix
 */
void RuntimeQML::addSuffix(const QString &suffix)
{
    if (m_allowedSuffixList.contains(suffix))
        return;

    m_allowedSuffixList.append(suffix);

    if (m_autoReload)
        loadQrcFiles();
}


/*!
 * \brief Reload the QML. Do not call it directly, use reload() instead.
 */
void RuntimeQML::reloadQml()
{
    if (m_mainQmlFilename.isEmpty()) {
        qCWarning(log, "Reloading: can't do, main QML file not specified.");
        return;
    }

    if (!m_noDebug)
        qCDebug(log, "Reloading...");

    if (!m_window) {
        auto rootObjects { m_engine->rootObjects() };
        if (!rootObjects.isEmpty())
            m_window = qobject_cast<QQuickWindow*>(rootObjects.back());
    }

    if (m_window) {
        if (m_closeAllOnReload) {
            // Find all child windows and close them
            auto const allWindows = m_window->findChildren<QQuickWindow*>();
            for (int i {0}; i < allWindows.size(); ++i) {
                QQuickWindow* w = qobject_cast<QQuickWindow*>(allWindows.at(i));
                if (w)
                    w->close();
            }
        }

        m_window->close();
        delete m_window; // Hopefully all goes well...
        m_window = nullptr;
        //m_window->deleteLater(); // FIXME ? Causes type errors on Qt 5.13+
    }

    m_engine->clearComponentCache();
    // TODO: test with network files
    // TODO: QString path to QUrl doesn't work under Windows with load() (load fail)
    m_engine->load(m_selector.select(qrcAbsolutePath() + "/" + m_mainQmlFilename));
    // NOTE: QQmlApplicationEngine::rootObjects() isn't cleared, should it be?

    emit reloaded();

//    for (auto *o : m_engine->rootObjects()) {
//        qDebug() << "> " << o;
//    }
}

/*!
 * \brief Called when a watched file changed, from QFileSystemWatcher.
 * \param path Path/file that triggered the signal.
 */
void RuntimeQML::fileChanged(const QString& path)
{
    if (!m_noDebug)
        qCDebug(log) << "File changed:" << path;
    reload();

#if defined(Q_OS_WIN) || (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    // Deleted files are removed from the watcher, re-add the file for
    //  systems that delete files to update them
    // Seems to happen on Windows and Linux
    if (m_fileWatcher)
        QTimer::singleShot(500, m_fileWatcher, [this,path](){
             m_fileWatcher->addPath(path);
        });
#endif
}


/*!
 * \brief Load qml from the QRC file to watch them.
 */
void RuntimeQML::loadQrcFiles()
{
    unloadFileWatcher();

    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &RuntimeQML::fileChanged);

    QFile file(m_qrcFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(log, "Unable to open resource file '%s', RuntimeQML will not work! Error: %s",
                 qPrintable(m_qrcFilename), qPrintable(file.errorString()));
        return;
    }

    QString const basePath = qrcAbsolutePath() + "/";
    QString currentPrefix;

    // Read each entry
    QXmlStreamReader inputStream(&file);
    while (!inputStream.atEnd() && !inputStream.hasError()) {
        inputStream.readNext();
        if (inputStream.isStartElement()) {
            QString name { inputStream.name().toString() };

            // Check prefix
            if (name == "qresource") {
                if (inputStream.attributes().hasAttribute("prefix")) {
                    auto p = inputStream.attributes().value("prefix").toString();
                    if (m_prefixIgnoreList.contains(p)) {
                        // Ignore this prefix, loop through elements in this 'qresource' tag
                        while (!inputStream.atEnd() && !inputStream.hasError()) {
                            inputStream.readNext();
                            if (inputStream.isEndElement() && inputStream.name().toString() == "qresource")
                                break;
                        }
                        continue;
                    }
                    currentPrefix = p;
                }
            }

            // Check file name
            if (name == "file") {
                QString const filename { inputStream.readElementText() };

                // Check ignore list
                QString const fullFilename { currentPrefix + filename };
                auto it = std::find_if(m_fileIgnoreList.cbegin(), m_fileIgnoreList.cend(), [&](QString const& pattern) {
                    QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern));
                    return re.match(fullFilename).hasMatch();
                });

                if (it != m_fileIgnoreList.cend())
                    continue;

                QFileInfo const file { filename };

                // Add to the watch list if the file suffix is allowed
                if (m_allowedSuffixList.contains(file.suffix())) {
                    QString fp { m_selector.select(basePath + filename) };
                    m_fileWatcher->addPath(fp);
                    //qDebug() << "    " << file.absoluteFilePath() << fp;
                }
            }
        }
    }

    if (!m_noDebug) {
        qCDebug(log, "Watching QML files:");
        int const fileCount = m_fileWatcher->files().size();

        for (auto &f : m_fileWatcher->files()) {
            qCDebug(log) << "    " << f;
        }

        if (fileCount > 0)
            qCDebug(log, "  Total: %d", fileCount);
        else
            qCDebug(log, "  None.");
    }
}

/*!
 * \brief Unload the file watcher.
 */
void RuntimeQML::unloadFileWatcher()
{
    if (m_fileWatcher) {
        disconnect(m_fileWatcher);
        delete m_fileWatcher;
        m_fileWatcher = nullptr;
    }
}
