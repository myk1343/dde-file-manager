/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
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
#include "processdialog.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QProcess>

#include <unistd.h>
#include <signal.h>

DWIDGET_USE_NAMESPACE
using namespace dfm_upgrade;

ProcessDialog::ProcessDialog(QWidget *parent)
    : DDialog(parent)
{
}

void ProcessDialog::initialize(bool desktop)
{
    onDesktop = desktop;

    if (desktop)
        setMessage(tr("File Manager will be updated to a new version, during which the tasks in progress will be terminated. Do you want to update now?"));
    else
        setMessage(tr("The desktop services will be updated to a new version, during which the tasks in progress will be terminated. Do you want to update now?"));
    accept = addButton(tr("Update", "button"), true, DDialog::ButtonWarning);
    addButton(tr("Cancel", "button"));

    setIcon(QIcon::fromTheme("dde-file-manager"));
}

bool ProcessDialog::execDialog()
{
    const QString exe = onDesktop ? QString("/usr/bin/dde-file-manager") : QString("/usr/bin/dde-desktop");
    auto process = queryProcess(exe);
    if (process.isEmpty())
        return true;

    if (exec() == accept) {
        killAll(process);
        killed = true;
        return true;
    }
    return false;
}

void ProcessDialog::restart()
{
    if (killed && !onDesktop) {
        const QString &desktop = "/usr/bin/dde-desktop";
        qInfo() << "restart desktop...";
#if (QT_VERSION <= QT_VERSION_CHECK(5, 15, 0))
        QProcess::startDetached(desktop);
#else
        QProcess::startDetached(desktop, {});
#endif
    }
}

QList<int> ProcessDialog::queryProcess(const QString &exec)
{
    int currentUser = getuid();
    QList<int> pids;
    QDir dir("/proc");
    auto entryInfos = dir.entryInfoList(QDir::Dirs);
    for (auto info : entryInfos) {
        QString name = info.fileName();
        bool ok = false;
        int pid = name.toInt(&ok);
        if (!ok || pid < 0)
            continue;

        auto exePath = targetExe(info.absoluteFilePath());
        if (isEqual(exePath, exec)) {
            int tuid = targetUid(info.absoluteFilePath());
            if (tuid == currentUser) {
                qInfo() << "find active process:" << exePath << pid << "user" << tuid;
                pids.append(pid);
            } else {
                qInfo() << "find anthoer user's active process:" << exePath << pid << "user" << tuid << currentUser;
            }
        }
    }

    return pids;
}

void ProcessDialog::killAll(const QList<int> &pids)
{
    for (int pid : pids)
        kill(pid, SIGKILL);
}

QString ProcessDialog::targetExe(const QString &proc)
{
    QFileInfo exeFile(proc + "/exe");
#if (QT_VERSION <= QT_VERSION_CHECK(5, 13, 0))
    return exeFile.readLink();
#else
    return exeFile.symLinkTarget();
#endif
}

int ProcessDialog::targetUid(const QString &proc)
{
    int ret = -2;
    QFile loginUid(proc + "/loginuid");
    if (loginUid.open(QFile::ReadOnly)) {
        QString str = loginUid.readAll();
        bool ok = false;
        int tmp = str.toInt(&ok);
        if (ok)
            ret = tmp;
    }

    return ret;
}

bool ProcessDialog::isEqual(const QString &link, QString match) const
{
    if (link == match)
        return true;

    // the exePath will contain suffix (deleted) like /usr/bin/dde-desktop (deleted) after upgrading.
    //! It is not sure that the suffix '(deleted)' is stable.
    match.append(" (deleted)");
    if (link == match) {
        qWarning() << "unstable match:" << match;
        return true;
    }

    return false;
}
