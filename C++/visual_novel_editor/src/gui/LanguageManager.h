#pragma once

#include <QObject>
#include <QTranslator>
#include <memory>

class QCoreApplication;

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    enum class Language {
        English,
        Chinese,
    };

    static LanguageManager &instance();

    void initialize(QCoreApplication *app);

    Language language() const;
    void setLanguage(Language language);

signals:
    void languageChanged(Language language);

private:
    LanguageManager();

    QCoreApplication *m_app{nullptr};
    std::unique_ptr<QTranslator> m_chineseTranslator;
    Language m_language{Language::English};
};

