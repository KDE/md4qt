
/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

#define MD4QT_QT_SUPPORT
#define MD4QT_ICU_STL_SUPPORT
#include <md4qt/doc.h>
#include <md4qt/parser.h>
#include <md4qt/html.h>

#include <QFile>
#include <QTest>
#include <QObject>

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm.h>
#include <registry.h>

class MdBenchmark : public QObject
{
	Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_stdStream.open("tests/manual/complex.md", std::ios::in | std::ios::binary);
        m_stdWd = std::filesystem::canonical(std::filesystem::current_path()).u8string();
        m_stdFileName = MD::UnicodeStringTrait::latin1ToString("tests/manual/complex.md");

        if (!m_stdStream.good()) {
            QFAIL("Unable to open file with STL.");
        }

        m_qtFileName = MD::QStringTrait::latin1ToString("tests/manual/complex.md");
        m_qtWd = QDir().absolutePath();

        QFile file(m_qtFileName);

        if (file.open(QIODeviceBase::ReadOnly)) {
            m_qtData = file.readAll();
            file.close();
        } else {
            QFAIL("Unable to open file with Qt.");
        }
    }

    void cleanupTestCase()
    {
        m_stdStream.close();
    }

    void md4qt_with_icu()
    {
        QBENCHMARK {
            if (m_stdStream.good()) {
                MD::Parser<MD::UnicodeStringTrait> parser;

                parser.parse(m_stdStream, m_stdWd, m_stdFileName, false);

                m_stdStream.seekg(std::ios::beg);
            }
        }
    }

    void md4qt_with_qt6()
    {
        QBENCHMARK {
            MD::Parser<MD::QStringTrait> parser;

            QTextStream stream(m_qtData);

            parser.parse(stream, m_qtWd, m_qtFileName, false);
        }
    }

    void md4qt_to_html()
    {
        MD::Parser<MD::QStringTrait> parser;

        const auto doc = parser.parse(QStringLiteral("tests/manual/complex.md"), false);

        QBENCHMARK {
            MD::toHtml(doc);
        }
    }

    void md4qt_with_qt6_without_autolinks()
    {
        QBENCHMARK {
            MD::Parser<MD::QStringTrait> parser;
            parser.removeTextPlugin(MD::GitHubAutoLinkPluginID);

            QTextStream stream(m_qtData);

            parser.parse(stream, m_qtWd, m_qtFileName, false);
        }
    }

    void cmark_gfm()
    {
        QBENCHMARK {
            cmark_gfm_core_extensions_ensure_registered();

            auto doc = cmark_parse_document(m_qtData.constData(), m_qtData.size(), CMARK_OPT_FOOTNOTES);

            cmark_node_free(doc);

            cmark_release_plugins();
        }
    }

private:
    std::ifstream m_stdStream;
    MD::UnicodeString m_stdWd;
    MD::UnicodeString m_stdFileName;
    QString m_qtWd;
    QString m_qtFileName;
    QByteArray m_qtData;
};

QTEST_GUILESS_MAIN(MdBenchmark)

#include "main.moc"
