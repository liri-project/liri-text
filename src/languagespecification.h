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

#ifndef LANGUAGESPECIFICATION_H
#define LANGUAGESPECIFICATION_H

#include <QHash>
#include <QString>
#include <QSharedPointer>

#include "languagestyle.h"
#include "languagecontext.h"
#include "languagecontext.h"
#include "languagecontextsimple.h"

class LanguageSpecification
{
public:
    LanguageSpecification();
    ~LanguageSpecification();

    QString name;
    QHash<QString, QSharedPointer<LanguageStyle>> styles;
    QSharedPointer<LanguageContextSimple> mainContext;
};

#endif // LANGUAGESPECIFICATION_H
