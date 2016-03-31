/*
 * Copyright (c) 2011 Vlad Voicu
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

/*
 * Copyright (c) 2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <QDialog>

#include <Swift/QtUI/ui_QtSpellCheckerWindow.h>

namespace Swift {
    class SettingsProvider;
    class QtSpellCheckerWindow : public QDialog, protected Ui::QtSpellCheckerWindow {
        Q_OBJECT
        public:
            QtSpellCheckerWindow(SettingsProvider* settings, QWidget* parent = NULL);
        public slots:
            void handleChecker(bool state);
            void handleCancel();
            void handlePathButton();
            void handlePersonalPathButton();
            void handleApply();
        private slots:
            void shrinkWindow();
        private:
            void setEnabled(bool state);
            void setFromSettings();
            void showFiles(const QStringList& files);
            SettingsProvider* settings_;
            Ui::QtSpellCheckerWindow ui_;
    };
}
