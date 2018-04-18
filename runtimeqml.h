#ifndef RUNTIMEQML_H
#define RUNTIMEQML_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QFileSelector>
#include <QFileSystemWatcher>

#include <QDebug>


class RuntimeQML : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString qrcFilename READ qrcFilename WRITE setQrcFilename NOTIFY qrcFilenameChanged)
    Q_PROPERTY(bool autoReload READ autoReload WRITE setAutoReload NOTIFY autoReloadChanged)
    Q_PROPERTY(bool closeAllOnReload READ closeAllOnReload WRITE setCloseAllOnReload NOTIFY closeAllOnReloadChanged)

public:
    explicit RuntimeQML(QQmlApplicationEngine *engine, QString const& qrcFilename = QString(), QObject *parent = 0);

    // If using QQmlFileSelector with Loader
    Q_INVOKABLE QString adjustPath(QString qmlFile);
    Q_INVOKABLE QString qrcAbsolutePath() const;

    QString qrcFilename() const;

    bool autoReload() const;
    bool closeAllOnReload() const;

    QList<QString> const & prefixIgnoreList() const;
    QList<QString> const & fileIgnoreList() const;
    QList<QString> const & allowedSuffixes() const;

    void noDebug();

signals:
    void autoReloadChanged(bool autoReload);
    void qrcFilenameChanged(QString qrcFilename);
    void closeAllOnReloadChanged(bool closeAllOnReload);

public slots:
    void reload();
    void setWindow(QQuickWindow* window);

    void setQrcFilename(QString qrcFilename);
    void setMainQmlFilename(QString filename);

    void setAutoReload(bool autoReload);
    void setCloseAllOnReload(bool closeAllOnReload);

    void ignoreQrcPrefix(QString const& prefix);
    void ignoreFile(QString const& filename);

    void addSuffix(QString const& suffix);

private slots:
    void reloadQml();
    void fileChanged(const QString &path);

private:
    void loadQrcFiles();
    void unloadFileWatcher();

    QQmlApplicationEngine *m_engine {nullptr};
    QQuickWindow *m_window {nullptr};
    QFileSelector m_selector;

    QString m_qrcFilename;
    QString m_mainQmlFilename;

    bool m_autoReload {false};
    QFileSystemWatcher* m_fileWatcher {nullptr};
    QList<QString> m_prefixIgnoreList;
    QList<QString> m_fileIgnoreList;
    QList<QString> m_allowedSuffixList;
    bool m_noDebug {false};
    bool m_closeAllOnReload {true};
};

#endif // RUNTIMEQML_H
