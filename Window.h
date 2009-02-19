/***************************************************************************
 * Copyright (c) 2009 Enrico Ros                                           *
 *         2009 Enrico Ros <enrico.ros@gmail.com>                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person             *
 * obtaining a copy of this software and associated documentation          *
 * files (the "Software"), to deal in the Software without                 *
 * restriction, including without limitation the rights to use,            *
 * copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the               *
 * Software is furnished to do so, subject to the following                *
 * conditions:                                                             *
 *                                                                         *
 * The above copyright notice and this permission notice shall be          *
 * included in all copies or substantial portions of the Software.         *
 *                                                                         *
 ***************************************************************************/

#ifndef __Window_h__
#define __Window_h__

#include <QWidget>
#include <QPixmap>
#include "SimpleHinter.h"
class Capture;
class Classifier;
class Recognizer;
class QSettings;

namespace Ui { class WindowForm; }

class AppWindow : public QWidget
{
    Q_OBJECT
    public:
        AppWindow( QWidget * parent = 0 );
        ~AppWindow();

    private:
        void saveSettings();
        Ui::WindowForm * ui;
        Capture * m_capture;
        Classifier * m_classifier;
        Recognizer * m_recognizer;
        SimpleHinter * m_hinter;
        QSettings * m_settings;
        QPixmap m_invalidPixmap;
        HintResult m_lastHint;

    private Q_SLOTS:
        void slotOnTopChanged();
        void slotOffToggled();
        void slotCapParamsChanged();
        void slotRecParamsChanged();
        void slotProcessPixmap( const QPixmap & pixmap, const QPoint & cursor );
};

#endif
