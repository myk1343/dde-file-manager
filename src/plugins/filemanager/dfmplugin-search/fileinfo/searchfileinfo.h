/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liuzhangjian<liuzhangjian@uniontech.com>
 *
 * Maintainer: liuzhangjian<liuzhangjian@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SEARCHFILEINFO_H
#define SEARCHFILEINFO_H

#include "dfmplugin_search_global.h"

#include "dfm-base/interfaces/abstractfileinfo.h"

DFMBASE_USE_NAMESPACE
namespace dfmplugin_search {

class SearchFileInfoPrivate;
class SearchFileInfo : public AbstractFileInfo
{
    Q_GADGET
public:
    explicit SearchFileInfo(const QUrl &url);
    ~SearchFileInfo() override;
    virtual bool exists() const override;
    virtual bool isAttributes(const FileIsType type = FileIsType::kIsFile) const override;
    virtual qint64 size() const override;
    virtual QString nameInfo(const FileNameInfoType type = FileNameInfoType::kFileName) const override;

    // property for view
    virtual QString viewTip(const ViewType type = ViewType::kEmptyDir) const override;
};

using SearchFileInfoPointer = QSharedPointer<SearchFileInfo>;
}

#endif   // SEARCHFILEINFO_H
