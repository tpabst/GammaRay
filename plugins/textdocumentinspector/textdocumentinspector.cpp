/*
  textdocumentinspector.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "textdocumentinspector.h"

#include "textdocumentformatmodel.h"
#include "textdocumentmodel.h"

#include <core/metaobject.h>
#include <core/metaobjectrepository.h>
#include <core/objecttypefilterproxymodel.h>
#include <core/probeinterface.h>

#include <common/objectbroker.h>

#include <QAbstractTextDocumentLayout>
#include <QItemSelection>
#include <QTextDocument>
#include <QTextList>
#include <QTextTable>

using namespace GammaRay;

TextDocumentInspector::TextDocumentInspector(ProbeInterface *probe, QObject *parent)
    : QObject(parent)
{
    registerMetaTypes();

    auto documentFilter = new ObjectTypeFilterProxyModel<QTextDocument>(this);
    documentFilter->setSourceModel(probe->objectListModel());
    probe->registerModel(QStringLiteral("com.kdab.GammaRay.TextDocumentsModel"), documentFilter);
    m_documentsModel = documentFilter;

    m_documentSelectionModel = ObjectBroker::selectionModel(documentFilter);
    connect(m_documentSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(documentSelected(QItemSelection,QItemSelection)));

    m_textDocumentModel = new TextDocumentModel(this);
    probe->registerModel(QStringLiteral("com.kdab.GammaRay.TextDocumentModel"),
                         m_textDocumentModel);

    auto selectionModel = ObjectBroker::selectionModel(m_textDocumentModel);
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(documentElementSelected(QItemSelection,QItemSelection)));

    m_textDocumentFormatModel = new TextDocumentFormatModel(this);
    probe->registerModel(QStringLiteral("com.kdab.GammaRay.TextDocumentFormatModel"), m_textDocumentFormatModel);

    connect(probe->probe(), SIGNAL(objectSelected(QObject*,QPoint)), SLOT(objectSelected(QObject*)));
}

void TextDocumentInspector::objectSelected(QObject* obj)
{
    if (auto doc = qobject_cast<QTextDocument*>(obj)) {
        const auto indexList = m_documentsModel->match(m_documentsModel->index(0, 0),
                                  ObjectModel::ObjectRole,
                                  QVariant::fromValue<QObject*>(doc), 1,
                                  Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap);
        if (indexList.isEmpty())
            return;

        const auto index = indexList.first();
        m_documentSelectionModel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else if (auto docObj = qobject_cast<QTextObject*>(obj)) {
        objectSelected(docObj->document());
    } else if (auto layout = qobject_cast<QAbstractTextDocumentLayout*>(obj)) {
        objectSelected(layout->document());
    }
}

void TextDocumentInspector::documentSelected(const QItemSelection &selected,
                                             const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.isEmpty()) {
        m_textDocumentModel->setDocument(0);
        return;
    }

    const QModelIndex selectedRow = selected.first().topLeft();
    QObject *selectedObj = selectedRow.data(ObjectModel::ObjectRole).value<QObject *>();
    QTextDocument *doc = qobject_cast<QTextDocument *>(selectedObj);

    m_textDocumentModel->setDocument(doc);
}

void TextDocumentInspector::documentElementSelected(const QItemSelection &selected,
                                                    const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.isEmpty())
        return;
    const QModelIndex selectedRow = selected.first().topLeft();
    const QTextFormat f = selectedRow.data(TextDocumentModel::FormatRole).value<QTextFormat>();
    m_textDocumentFormatModel->setFormat(f);
}

void TextDocumentInspector::registerMetaTypes()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    MetaObject *mo = Q_NULLPTR;
    MO_ADD_METAOBJECT1(QTextObject, QObject);
    MO_ADD_PROPERTY_RO(QTextObject, QTextDocument*, document);
    MO_ADD_PROPERTY_RO(QTextObject, int, formatIndex);
    MO_ADD_PROPERTY_RO(QTextObject, int, objectIndex);

    MO_ADD_METAOBJECT1(QTextFrame, QTextObject);
    MO_ADD_PROPERTY_RO(QTextFrame, int, firstPosition);
    MO_ADD_PROPERTY_RO(QTextFrame, int, lastPosition);
    MO_ADD_PROPERTY_RO(QTextFrame, QTextFrame*, parentFrame);

    MO_ADD_METAOBJECT1(QTextTable, QTextFrame);
    MO_ADD_PROPERTY_RO(QTextTable, int, columns);
    MO_ADD_PROPERTY_RO(QTextTable, int, rows);

    MO_ADD_METAOBJECT1(QTextBlockGroup, QTextObject);
    MO_ADD_METAOBJECT1(QTextList, QTextBlockGroup);
    MO_ADD_PROPERTY_RO(QTextList, int, count);

    MO_ADD_METAOBJECT1(QAbstractTextDocumentLayout, QObject);
    MO_ADD_PROPERTY_RO(QAbstractTextDocumentLayout, QTextDocument*, document);
    MO_ADD_PROPERTY_RO(QAbstractTextDocumentLayout, QSizeF, documentSize);
    MO_ADD_PROPERTY_RO(QAbstractTextDocumentLayout, int, pageCount);
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN(TextDocumentInspectorFactory)
#endif
