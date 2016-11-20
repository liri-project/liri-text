/*
 * Copyright © 2016 Andrew Penkrat
 *
 * This file is part of Liri Text.
 *
 * Liri Text is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Liri Text is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Liri Text.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "languageloader.h"
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include "languagecontextkeyword.h"
#include "languagecontextsimple.h"
#include "languagecontextcontainer.h"
#include "languagecontextsubpattern.h"

LanguageLoader::LanguageLoader(QSharedPointer<LanguageDefaultStyles> defaultStyles) :
    knownContexts(),
    knownStyles(),
    knownRegexes() {
    for (auto styleId : defaultStyles->styles.keys()) {
        knownStyles[styleId] = QSharedPointer<LanguageStyle>();
    }
}

LanguageLoader::~LanguageLoader() { }

QSharedPointer<LanguageSpecification> LanguageLoader::loadById(QString name) {
    qDebug() << "Loading " << name;
    // TODO: Build languages database, querry it here for path to spec
    return loadFromFile(QString("/usr/share/gtksourceview-3.0/language-specs/%1.lang").arg(name));
}

QSharedPointer<LanguageSpecification> LanguageLoader::loadFromFile(QString path) {
    QSharedPointer<LanguageSpecification> result = QSharedPointer<LanguageSpecification>(new LanguageSpecification());
    QFile file(path);
    if(file.open(QFile::ReadOnly)) {
        QXmlStreamReader xml(&file);
        while (!xml.atEnd()) {
            xml.readNext();
            if(xml.name() == "language" && xml.tokenType() == QXmlStreamReader::StartElement)
                result->name = xml.attributes().value("id").toString();
            if(xml.name() == "metadata")
                parseMetadata(xml);
            if(xml.name() == "define-regex")
                parseDefineRegex(xml);
            if(xml.name() == "context")
                parseContext(xml, result->name);
            if(xml.name() == "style")
                parseStyle(xml, result->name);
        }
    }
    file.close();
    QString mainId = result->name + ":" + result->name;
    if(knownContexts.keys().contains(mainId))
        result->mainContext = knownContexts[mainId].staticCast<LanguageContextSimple>();
    return result;
}

void LanguageLoader::parseMetadata(QXmlStreamReader &xml) {
    xml.skipCurrentElement();
}

QSharedPointer<LanguageContext> LanguageLoader::parseContext(QXmlStreamReader &xml, QString langId) {
    QSharedPointer<LanguageContext> result = QSharedPointer<LanguageContext>();
    QString id = xml.attributes().value("id").toString();
    if(xml.attributes().hasAttribute("ref")) {
        QStringRef refId = xml.attributes().value("ref");
        if(refId.contains(':') && !knownContexts.keys().contains(refId.toString())) {
            loadById(refId.left(refId.indexOf(':')).toString());
        }
        QString refIdCopy = refId.toString();
        if(!refIdCopy.contains(':'))
            refIdCopy = langId + ":" + refIdCopy;
        if(knownContexts.keys().contains(refIdCopy))
            result = knownContexts[refIdCopy];
    }
    if(result && id != "") {
        knownContexts[langId + ":" + id] = result;
    }

    if(xml.attributes().hasAttribute("sub-pattern")) {
        result = QSharedPointer<LanguageContext>(new LanguageContextSubPattern());
        if(id != "")
            knownContexts[langId + ":" + id] = result;
        QSharedPointer<LanguageContextSubPattern> subpattern = result.staticCast<LanguageContextSubPattern>();

        subpattern->group = xml.attributes().value("sub-pattern").toInt();
        if(xml.attributes().value("where") == "start")
            subpattern->where = LanguageContextSubPattern::Start;
        if(xml.attributes().value("where") == "end")
            subpattern->where = LanguageContextSubPattern::End;
    }

    QString styleId = "";
    if(xml.attributes().hasAttribute("style-ref")) {
        QStringRef styleIdRef = xml.attributes().value("style-ref");
        if(styleIdRef.contains(':') && !knownStyles.keys().contains(styleIdRef.toString()))
            loadById(styleIdRef.left(styleIdRef.indexOf(':')).toString());
        styleId = styleIdRef.toString();
        if(!styleId.contains(':'))
            styleId = langId + ":" + styleId;
    }

    xml.readNext();
    while (xml.name() != "context" || xml.tokenType() != QXmlStreamReader::EndElement) {
        if(xml.name() == "start") {
            if(!result) {
                result = QSharedPointer<LanguageContext>(new LanguageContextContainer());
                if(id != "") {
                    knownContexts[langId + ":" + id] = result;
                }
            }
            QSharedPointer<LanguageContextContainer> container = result.staticCast<LanguageContextContainer>();
            xml.readNext();
            container->startPattern = resolveRegex(xml.text().toString());
            xml.readNext();
        }
        if(xml.name() == "end") {
            if(!result) {
                result = QSharedPointer<LanguageContext>(new LanguageContextContainer());
                if(id != "") {
                    knownContexts[langId + ":" + id] = result;
                }
            }
            QSharedPointer<LanguageContextContainer> container = result.staticCast<LanguageContextContainer>();
            xml.readNext();
            container->endPattern = resolveRegex(xml.text().toString());
            xml.readNext();
        }
        if(xml.name() == "match") {
            if(!result) {
                result = QSharedPointer<LanguageContext>(new LanguageContextSimple());
                if(id != "") {
                    knownContexts[langId + ":" + id] = result;
                }
            }
            xml.readNext();
            QSharedPointer<LanguageContextSimple> simple = result.staticCast<LanguageContextSimple>();
            simple->matchPattern = resolveRegex(xml.text().toString());
            xml.readNext();
        }
        if(xml.name() == "keyword") {
            if(!result) {
                result = QSharedPointer<LanguageContext>(new LanguageContextKeyword());
                if(id != "") {
                    knownContexts[langId + ":" + id] = result;
                }
            }
            QSharedPointer<LanguageContextKeyword> kw = result.staticCast<LanguageContextKeyword>();
            xml.readNext();
            kw->keywords.append(xml.text().toString());
            xml.readNext();
        }
        if(xml.name() == "include") {
            xml.readNext();
            while (xml.name() != "include" || xml.tokenType() != QXmlStreamReader::EndElement) {
                if(xml.name() == "context") {
                    QSharedPointer<LanguageContext> inc = parseContext(xml, langId);
                    if(!result) {
                        result = QSharedPointer<LanguageContext>(new LanguageContextSimple());
                        if(id != "") {
                            knownContexts[langId + ":" + id] = result;
                        }
                    }

                    if(result->type == LanguageContext::Simple) {
                        QSharedPointer<LanguageContextSimple> simple = result.staticCast<LanguageContextSimple>();
                        if(inc)
                            simple->includes.append(inc);
                    } else if(result->type == LanguageContext::Container) {
                        QSharedPointer<LanguageContextContainer> container = result.staticCast<LanguageContextContainer>();
                        if(inc)
                            container->includes.append(inc);
                    } else {
                        Q_ASSERT(false);
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }

    if(knownStyles.keys().contains(styleId)) {
        if(knownStyles[styleId])
            result->style = knownStyles[styleId];
        else {
            result->style = QSharedPointer<LanguageStyle>(new LanguageStyle());
            result->style->defaultId = styleId;
        }
    }

    return result;
}

QSharedPointer<LanguageStyle> LanguageLoader::parseStyle(QXmlStreamReader &xml, QString langId) {
    QSharedPointer<LanguageStyle> result = QSharedPointer<LanguageStyle>();
    QString id = xml.attributes().value("id").toString();
    if(xml.attributes().hasAttribute("map-to")) {
        QStringRef refId = xml.attributes().value("map-to");
        if(refId.contains(':') && !knownStyles.keys().contains(refId.toString())) {
            loadById(refId.left(refId.indexOf(':')).toString());
        }
        QString refIdCopy = refId.toString();
        if(!refIdCopy.contains(':'))
            refIdCopy = langId + ":" + refIdCopy;
        if(knownStyles.keys().contains(refIdCopy)) {
            if(knownStyles[refIdCopy])
                result = knownStyles[refIdCopy];
            else {
                result = QSharedPointer<LanguageStyle>(new LanguageStyle());
                result->defaultId = refIdCopy;
                knownStyles[langId + ":" + id] = result;
            }
        }
    }
    if(result && id != "") {
        knownStyles[langId + ":" + id] = result;
    }
    xml.skipCurrentElement();
    return result;
}

void LanguageLoader::parseDefineRegex(QXmlStreamReader &xml) {
    QString id = xml.attributes().value("id").toString();
    xml.readNext();
    knownRegexes[id] = resolveRegex(xml.text().toString());
    xml.readNext();
}

QString LanguageLoader::resolveRegex(QString pattern) {
    QString result = pattern;

    QRegularExpression whiteSpace("(\\s+#[^\n]*\n\\s*|\\s*\n\\s*|(?<!\\\\)\\s)");
    result.replace(whiteSpace, "");

    result = result.replace("\\%[", "\\b");
    result = result.replace("\\%]", "\\b");
    for (QString id : knownRegexes.keys()) {
        result = result.replace("\\%{" + id + "}", knownRegexes[id]);
    }
    return result;
}
