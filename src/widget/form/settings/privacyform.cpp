/*
    Copyright © 2014-2019 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "privacyform.h"
#include "ui_privacysettings.h"

#include <QDebug>
#include <QFile>
#include <QMessageBox>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#else
#include <QDateTime>
#endif

#include "src/core/core.h"
#include "src/nexus.h"
#include "src/persistence/history.h"
#include "src/persistence/profile.h"
#include "src/persistence/settings.h"
#include "src/widget/form/setpassworddialog.h"
#include "src/widget/form/settingswidget.h"
#include "src/widget/gui.h"
#include "src/widget/tool/recursivesignalblocker.h"
#include "src/widget/translator.h"
#include "src/widget/widget.h"

PrivacyForm::PrivacyForm(Core* _core)
    : GenericForm(QPixmap(":/img/settings/privacy.png"))
    , bodyUI(new Ui::PrivacySettings)
    , core{_core}
{
    bodyUI->setupUi(this);

    // block all child signals during initialization
    const RecursiveSignalBlocker signalBlocker(this);

    eventsInit();
    Translator::registerHandler(std::bind(&PrivacyForm::retranslateUi, this), this);
}

PrivacyForm::~PrivacyForm()
{
    Translator::unregister(this);
    delete bodyUI;
}

void PrivacyForm::on_cbKeepHistory_stateChanged()
{
    Settings::getInstance().setEnableLogging(bodyUI->cbKeepHistory->isChecked());
    if (!bodyUI->cbKeepHistory->isChecked()) {
        emit clearAllReceipts();
        QMessageBox::StandardButton dialogDelHistory;
        dialogDelHistory =
            QMessageBox::question(nullptr, tr("Confirmation"),
                                  tr("Do you want to permanently delete all chat history?"),
                                  QMessageBox::Yes | QMessageBox::No);
        if (dialogDelHistory == QMessageBox::Yes) {
            Nexus::getProfile()->getHistory()->eraseHistory();
        }
    }
}

void PrivacyForm::on_cbTypingNotification_stateChanged()
{
    Settings::getInstance().setTypingNotification(bodyUI->cbTypingNotification->isChecked());
}

void PrivacyForm::on_nospamLineEdit_editingFinished()
{
    QString newNospam = bodyUI->nospamLineEdit->text();

    bool ok;
    uint32_t nospam = newNospam.toLongLong(&ok, 16);
    if (ok) {
        core->setNospam(nospam);
    }
}

void PrivacyForm::showEvent(QShowEvent*)
{
    const Settings& s = Settings::getInstance();
    bodyUI->nospamLineEdit->setText(core->getSelfId().getNoSpamString());
    bodyUI->cbTypingNotification->setChecked(s.getTypingNotification());
    bodyUI->cbKeepHistory->setChecked(Settings::getInstance().getEnableLogging());
    bodyUI->blackListTextEdit->setText(s.getBlackList().join('\n'));
}

void PrivacyForm::on_randomNosapamButton_clicked()
{
    uint32_t newNospam{0};

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    // guarantees to give a random 32-bit unsigned integer
    newNospam = QRandomGenerator::global()->generate();
#else
    qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));
    for (int i = 0; i < 4; ++i)
        // Generate byte by byte, as qrand() is guaranteed to have only 15 bits of randomness (RAND_MAX is guaranteed to be 2^15)
        newNospam = (newNospam << 8) + (static_cast<int>((static_cast<double>(qrand()) / static_cast<double>(RAND_MAX+1l)) * 256));
#endif

    core->setNospam(newNospam);
    bodyUI->nospamLineEdit->setText(core->getSelfId().getNoSpamString());
}

void PrivacyForm::on_nospamLineEdit_textChanged()
{
    QString str = bodyUI->nospamLineEdit->text();
    int curs = bodyUI->nospamLineEdit->cursorPosition();
    if (str.length() != 8) {
        str = QString("00000000").replace(0, str.length(), str);
        bodyUI->nospamLineEdit->setText(str);
        bodyUI->nospamLineEdit->setCursorPosition(curs);
    }
}

void PrivacyForm::on_blackListTextEdit_textChanged()
{
    const QStringList strlist = bodyUI->blackListTextEdit->toPlainText().split('\n');
    Settings::getInstance().setBlackList(strlist);
}

void PrivacyForm::retranslateUi()
{
    bodyUI->retranslateUi(this);
}
