#pragma once

#include <QObject>
#include <memory>

class QCoreApplication;

class InlineTranslator;

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
    std::unique_ptr<InlineTranslator> m_chineseTranslator;
    Language m_language{Language::English};
};

