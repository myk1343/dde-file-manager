/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

#include "docutfilesworker.h"
#include "fileoperations/fileoperationutils/fileoperationsutils.h"
#include "fileoperations/copyfiles/storageinfo.h"

#include "dfm-base/base/schemefactory.h"
#include "dfm-base/utils/fileutils.h"
#include "dfm-base/utils/decorator/decoratorfile.h"
#include "dfm-base/utils/decorator/decoratorfileinfo.h"

#include "dfm-io/core/diofactory.h"

#include <QUrl>
#include <QProcess>
#include <QMutex>
#include <QStorageInfo>
#include <QQueue>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syscall.h>

DPFILEOPERATIONS_USE_NAMESPACE
DoCutFilesWorker::DoCutFilesWorker(QObject *parent)
    : FileOperateBaseWorker(parent)
{
    jobType = AbstractJobHandler::JobType::kCutType;
}

DoCutFilesWorker::~DoCutFilesWorker()
{
    stop();
}

bool DoCutFilesWorker::doWork()
{
    // The endcopy interface function has been called here
    if (!AbstractWorker::doWork())
        return false;

    // check progress notify type
    determineCountProcessType();

    // 执行剪切
    if (!cutFiles()) {
        endWork();
        return false;
    }

    // sync
    syncFilesToDevice();

    // 完成
    endWork();

    return true;
}

void DoCutFilesWorker::stop()
{
    AbstractWorker::stop();
}

bool DoCutFilesWorker::initArgs()
{
    time.start();

    AbstractWorker::initArgs();

    if (sourceUrls.count() <= 0) {
        // pause and emit error msg
        doHandleErrorAndWait(QUrl(), QUrl(), AbstractJobHandler::JobErrorType::kProrogramError);
        return false;
    }
    if (!targetUrl.isValid()) {
        // pause and emit error msg
        doHandleErrorAndWait(sourceUrls.first(), targetUrl, AbstractJobHandler::JobErrorType::kProrogramError);
        return false;
    }
    targetInfo = InfoFactory::create<AbstractFileInfo>(targetUrl);
    if (!targetInfo) {
        // pause and emit error msg
        doHandleErrorAndWait(sourceUrls.first(), targetUrl, AbstractJobHandler::JobErrorType::kProrogramError);
        return false;
    }

    if (!DecoratorFile(targetUrl).exists()) {
        // pause and emit error msg
        doHandleErrorAndWait(sourceUrls.first(), targetUrl, AbstractJobHandler::JobErrorType::kNonexistenceError);
        return false;
    }

    targetStorageInfo.reset(new StorageInfo(targetUrl.path()));

    return true;
}

bool DoCutFilesWorker::cutFiles()
{
    for (const auto &url : sourceUrls) {
        if (!stateCheck()) {
            return false;
        }

        const auto &fileInfo = InfoFactory::create<AbstractFileInfo>(url);
        if (!fileInfo) {
            // pause and emit error msg
            if (AbstractJobHandler::SupportAction::kSkipAction != doHandleErrorAndWait(url, targetUrl, AbstractJobHandler::JobErrorType::kProrogramError)) {
                return false;
            } else {
                continue;
            }
        }
        fileInfo->refresh();

        // check self
        if (checkSelf(fileInfo))
            continue;

        // check hierarchy
        if (fileInfo->isAttributes(AbstractFileInfo::FileIsType::kIsDir)) {
            const bool higher = FileUtils::isHigherHierarchy(url, targetUrl) || url == targetUrl;
            if (higher) {
                emit requestShowTipsDialog(DFMBASE_NAMESPACE::AbstractJobHandler::ShowDialogType::kCopyMoveToSelf, {});
                return false;
            }
        }

        // check link
        if (fileInfo->isAttributes(AbstractFileInfo::FileIsType::kIsSymLink)) {
            const bool ok = checkSymLink(fileInfo);
            if (ok)
                continue;
            else
                return false;
        }

        if (!doCutFile(fileInfo, targetInfo)) {
            return false;
        }
        fileInfo->refresh();
    }
    return true;
}

bool DoCutFilesWorker::doCutFile(const AbstractFileInfoPointer &fromInfo, const AbstractFileInfoPointer &targetPathInfo)
{
    // try rename
    bool ok = false;
    AbstractFileInfoPointer toInfo = nullptr;
    if (doRenameFile(fromInfo, targetPathInfo, toInfo, &ok) || ok) {
        currentWriteSize += fromInfo->size();
        return true;
    }

    if (stopWork.load()) {
        stopWork.store(false);
        return false;
    }

    qDebug() << "do rename failed, use copy and delete way, from url: " << fromInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl) << " to url: " << targetPathInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl);

    bool result = false;
    // check space
    if (!checkDiskSpaceAvailable(fromInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl), targetPathInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl), targetStorageInfo, &result))
        return result;

    if (!copyAndDeleteFile(fromInfo, targetPathInfo, toInfo, &result))
        return result;

    currentWriteSize += fromInfo->size();
    return true;
}

void DoCutFilesWorker::onUpdateProgress()
{
    const qint64 writSize = getWriteDataSize();
    emitProgressChangedNotify(writSize);
    emitSpeedUpdatedNotify(writSize);
}

void DoCutFilesWorker::emitCompleteFilesUpdatedNotify(const qint64 &writCount)
{
    JobInfoPointer info(new QMap<quint8, QVariant>);
    info->insert(AbstractJobHandler::NotifyInfoKey::kCompleteFilesKey, QVariant::fromValue(writCount));

    emit stateChangedNotify(info);
}

bool DoCutFilesWorker::checkSymLink(const AbstractFileInfoPointer &fileInfo)
{
    const QUrl &sourceUrl = fileInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl);
    AbstractFileInfoPointer newTargetInfo(nullptr);
    bool result = false;
    bool ok = doCheckFile(fileInfo, targetInfo, fileInfo->nameInfo(AbstractFileInfo::FileNameInfoType::kFileCopyName),
                          newTargetInfo, &result);
    if (!ok && !result)
        return false;
    ok = createSystemLink(fileInfo, newTargetInfo, true, false, &result);
    if (!ok && !result)
        return false;
    ok = deleteFile(sourceUrl, QUrl(), &result);
    if (!ok && !result)
        return false;

    completeSourceFiles.append(sourceUrl);
    completeTargetFiles.append(newTargetInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl));

    return true;
}

bool DoCutFilesWorker::checkSelf(const AbstractFileInfoPointer &fileInfo)
{
    const QString &fileName = fileInfo->nameInfo(AbstractFileInfo::FileNameInfoType::kFileName);
    QString newFileUrl = targetInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl).toString();
    if (!newFileUrl.endsWith("/"))
        newFileUrl.append("/");
    newFileUrl.append(fileName);
    DecoratorFileInfo newFileInfo(QUrl(newFileUrl, QUrl::TolerantMode));

    if (newFileInfo.url() == fileInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl)
        || (FileUtils::isSameFile(fileInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl), newFileInfo.url()) && !fileInfo->isAttributes(AbstractFileInfo::FileIsType::kIsSymLink))) {
        return true;
    }
    return false;
}

bool DoCutFilesWorker::renameFileByHandler(const AbstractFileInfoPointer &sourceInfo, const AbstractFileInfoPointer &targetInfo)
{
    if (localFileHandler) {
        const QUrl &sourceUrl = sourceInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl);
        const QUrl &targetUrl = targetInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl);
        return localFileHandler->renameFile(sourceUrl, targetUrl);
    }
    return false;
}

bool DoCutFilesWorker::doRenameFile(const AbstractFileInfoPointer &sourceInfo, const AbstractFileInfoPointer &targetPathInfo, AbstractFileInfoPointer &toInfo, bool *ok)
{
    QSharedPointer<QStorageInfo> sourceStorageInfo = nullptr;
    sourceStorageInfo.reset(new QStorageInfo(sourceInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl).path()));

    const QUrl &sourceUrl = sourceInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl);

    toInfo.reset();
    if (sourceStorageInfo->device() == targetStorageInfo->device()) {
        if (!doCheckFile(sourceInfo, targetPathInfo, sourceInfo->nameInfo(AbstractFileInfo::FileNameInfoType::kFileCopyName),
                         toInfo, ok))
            return ok ? *ok : false;

        emitCurrentTaskNotify(sourceInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl), toInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl));
        bool result = renameFileByHandler(sourceInfo, toInfo);
        if (result) {
            if (targetPathInfo == this->targetInfo) {
                completeSourceFiles.append(sourceUrl);
                completeTargetFiles.append(toInfo->urlInfo(AbstractFileInfo::FileUrlInfoType::kUrl));
            }
        }
        if (ok)
            *ok = result;
        return result;
    }

    if (!toInfo && !doCheckFile(sourceInfo, targetPathInfo, sourceInfo->nameInfo(AbstractFileInfo::FileNameInfoType::kFileCopyName), toInfo, ok))
        return false;

    return false;
}
