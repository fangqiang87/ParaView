/*=========================================================================

   Program: ParaView
   Module:    pqTranslationsTesting.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqTranslationsTesting.h"
#include "pqCoreUtilities.h"
#include "pqObjectNaming.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QMainWindow>
#include <QMenu>
#include <QRegularExpression>
#include <QStack>
#include <QWindow>

//-----------------------------------------------------------------------------
pqTranslationsTesting::pqTranslationsTesting(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
pqTranslationsTesting::~pqTranslationsTesting() = default;

//-----------------------------------------------------------------------------
bool pqTranslationsTesting::isIgnored(QObject* object, const char* property) const
{
  for (QPair<QString, QString> pair : TRANSLATION_IGNORE_STRINGS)
  {
    if (pqObjectNaming::GetName(*object) == pair.first &&
      (QString(property) == pair.second || QString(pair.second).isEmpty()))
    {
      return true;
    }
  }
  for (QPair<QRegularExpression, QString> pair : TRANSLATION_IGNORE_REGEXES)
  {
    if (pqObjectNaming::GetName(*object).contains(pair.first) &&
      (QString(property) == pair.second || QString(pair.second).isEmpty()))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::printWarningIfUntranslated(QObject* object, const char* property) const
{
  QString str = object->property(property).toString();
  bool isNumber;
  // Replace commas by point so `toDouble` can detect numbers
  str.replace(QRegularExpression(","), QString("."));
  str.toDouble(&isNumber);
  if (isNumber)
  {
    return;
  }

  // Remove HTML tags
  str.remove(QRegularExpression("<[\\/]?[aZ-zA-Z]+ ?[^>]*[\\/]?>"));
  // Remove & before _TranslationTestings
  str.replace(QRegularExpression("&(_TranslationTesting)"), QString("\\1"));
  // Remove <>, [] and () around expressions
  str.replace(QRegularExpression("[\\[{<\\(](.*)[\\]}>\\)]"), QString("\\1"));
  // isIgnored can be slow because of regex usage, so we do it at last
  if (!str.contains(QRegularExpression("^(_TranslationTesting.*)?$")) &&
    !this->isIgnored(object, property))
  {

    qCritical() << QString("Error: %2 of %1 not translated : '%3'")
                     .arg(pqObjectNaming::GetName(*object), property, str);
    return;
  }
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::recursiveFindUntranslatedStrings(QWidget* widget) const
{
  this->printWarningIfUntranslated(widget, "text");
  this->printWarningIfUntranslated(widget, "toolTip");
  this->printWarningIfUntranslated(widget, "windowTitle");
  this->printWarningIfUntranslated(widget, "placeholderText");
  this->printWarningIfUntranslated(widget, "label");

  for (QAction* action : widget->actions())
  {
    this->printWarningIfUntranslated(action, "text");
    this->printWarningIfUntranslated(action, "toolTip");
    this->printWarningIfUntranslated(action, "windowTitle");
    this->printWarningIfUntranslated(action, "placeholderText");
    this->printWarningIfUntranslated(action, "label");
  }
  for (QWidget* child :
    widget->findChildren<QWidget*>(QRegularExpression("^.*$"), Qt::FindDirectChildrenOnly))
  {
    this->recursiveFindUntranslatedStrings(child);
  }
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::onStartup()
{
  QMainWindow* win = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  this->recursiveFindUntranslatedStrings(win);
}
