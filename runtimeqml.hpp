#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QScopedPointer>

class RuntimeQmlPrivate;
class RuntimeQml : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoReload READ autoReload WRITE setAutoReload NOTIFY autoReloadChanged)

public:
    explicit RuntimeQml(QQmlApplicationEngine* engine, QObject* parent = nullptr);
    ~RuntimeQml() override;

    void parseQrc(QString const& qrcFilename);

    void load(QUrl const& url);
    void setReloadUrl(QUrl const& url);
    QUrl const& reloadUrl() const;

    //! Reload the main QML file.
    Q_INVOKABLE void reload();

    bool autoReload() const;
    void setAutoReload(bool autoReload);

    void addFileSuffixFilter(QString const& suffix);
    QList<QString> const& allowedSuffixes() const;

    void addIgnoreFilter(QString const& filter);
    void addQrcPrefixIgnoreFilter(QString const& prefix);

    QList<QString> const& ignoreFiltersList() const;

signals:
    void autoReloadChanged(bool autoReload);
    void reloaded();

private slots:
    void reloadQml();

private:
    Q_DECLARE_PRIVATE_D(dd_ptr, RuntimeQml)
    QScopedPointer<RuntimeQmlPrivate> dd_ptr;
};
