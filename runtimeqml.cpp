#include "runtimeqml.h"

#include <QXmlStreamReader>
#include <QDirIterator>


RuntimeQML::RuntimeQML(QQmlApplicationEngine* engine, const QString &qrcFilename, QObject *parent) :
    QObject(parent),
    m_engine(engine),
    m_qrcFilename(qrcFilename),
    m_mainQmlFilename("main.qml")
{
    m_allowedSuffixList << "qml";
}


QString RuntimeQML::adjustPath(QString qmlFile)
{
    return m_selector.select(qrcAbsolutePath() + "/" + qmlFile);
}

QString RuntimeQML::qrcAbsolutePath() const
{
    return QFileInfo(m_qrcFilename).absolutePath();
}

QString RuntimeQML::qrcFilename() const
{
    return m_qrcFilename;
}

bool RuntimeQML::autoReload() const
{
    return m_autoReload;
}

const QList<QString>& RuntimeQML::prefixIgnoreList() const
{
    return m_prefixIgnoreList;
}

const QList<QString> &RuntimeQML::fileIgnoreList() const
{
    return m_fileIgnoreList;
}

const QList<QString> &RuntimeQML::allowedSuffixes() const
{
    return m_allowedSuffixList;
}

void RuntimeQML::noDebug()
{
    if (m_noDebug) return;
    m_noDebug = true;
}


void RuntimeQML::reload()
{
    QMetaObject::invokeMethod(this, "reloadQml", Qt::QueuedConnection);
}


void RuntimeQML::setWindow(QQuickWindow* window)
{
    if (window == m_window) return;
    m_window = window;
}


void RuntimeQML::setQrcFilename(QString qrcFilename)
{
    if (m_qrcFilename == qrcFilename)
        return;

    m_qrcFilename = qrcFilename;
    emit qrcFilenameChanged(qrcFilename);

    loadQrcFiles();
}

void RuntimeQML::setMainQmlFilename(QString filename)
{
    if (m_mainQmlFilename == filename)
        return;

    m_mainQmlFilename = filename;
}


void RuntimeQML::setAutoReload(bool autoReload)
{
    if (m_autoReload == autoReload)
        return;

    m_autoReload = autoReload;
    emit autoReloadChanged(autoReload);

    if (autoReload) {
        loadQrcFiles();
    } else {
        unloadFileWatcher();
    }
}

void RuntimeQML::ignorePrefix(const QString& prefix)
{
    if (m_prefixIgnoreList.contains(prefix)) return;

    m_prefixIgnoreList.append(prefix);

    //qDebug() << "Ignoring prefix:" << prefix;

    if (m_autoReload) {
        loadQrcFiles();
    }
}

void RuntimeQML::ignoreFile(const QString &filename)
{
    if (m_fileIgnoreList.contains(filename)) return;

    m_fileIgnoreList.append(filename);

    //qDebug() << "Ignoring file:" << filename;

    if (m_autoReload) {
        loadQrcFiles();
    }
}

void RuntimeQML::addSuffix(const QString &suffix)
{
    if (m_allowedSuffixList.contains(suffix)) return;

    m_allowedSuffixList.append(suffix);

    //qDebug() << "Allowing file suffix:" << suffix;

    if (m_autoReload) {
        loadQrcFiles();
    }
}


void RuntimeQML::reloadQml()
{
    if (m_mainQmlFilename.isEmpty()) {
        qWarning("No QML file specified.");
        return;
    }

    if (m_window) {
        m_window->close();
    }

    m_engine->clearComponentCache();
    m_engine->load(QUrl(m_selector.select(qrcAbsolutePath() + "/" + m_mainQmlFilename)));
    // TODO: QQmlApplicationEngine::rootObjects() isn't cleared, should it be?

    if (!m_engine->rootObjects().isEmpty()) {
        QQuickWindow* w = qobject_cast<QQuickWindow*>(m_engine->rootObjects().last());
        if (w) m_window = w;
        //qDebug() << m_window << w->parent();
    }

//    for (auto *o : m_engine->rootObjects()) {
//        qDebug() << "> " << o;
//    }
}

void RuntimeQML::fileChanged(const QString& path)
{
    qDebug() << "Reloading qml:" << path;
    reload();
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
        qWarning("Unable to open resource file, RuntimeQML will not work! Error: %s", qPrintable(file.errorString()));
        return;
    }

    QString const basePath = qrcAbsolutePath() + "/";

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
                            if (inputStream.isEndElement() && inputStream.name() == "qresource")
                                break;
                        }
                        continue;
                    }
                }
            }

            // Check file name
            if (name == "file") {
                QString const filename { inputStream.readElementText() };

                // Check ignore list
                if (m_fileIgnoreList.contains(filename)) {
                    continue;
                }

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
        qDebug("Watching QML files:");
        int const fileCount = m_fileWatcher->files().size();
        for (auto &f : m_fileWatcher->files()) {
            qDebug() << "    " << f;
        }

        if (fileCount > 0) {
            qDebug("  Total: %d", fileCount);
        } else {
            qDebug("  None.");
        }
    }
}

void RuntimeQML::unloadFileWatcher()
{
    if (m_fileWatcher) {
        disconnect(m_fileWatcher);
        delete m_fileWatcher;
        m_fileWatcher = nullptr;
    }
}
