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

#include "Window.h"
#include "Capture.h"
#include "Classifier.h"
#include "Recognizer.h"
#include "ui_Window.h"

#include <QCursor>
#include <QDesktopWidget>
#include <QFile>
#include <QImage>
#include <QSettings>

#define DEFAULT_WIDTH 252
#define DEFAULT_HEIGHT 330

AppWindow::AppWindow( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::WindowForm )
#if defined(Q_OS_WIN)
    , m_settings( new QSettings( "biocheat.ini", QSettings::IniFormat ) )
#else
    , m_settings( new QSettings() )
#endif
    , m_invalidPixmap( ":/data/icon-invalid.png" )
{
    // create ui
    ui->setupUi( this );
    QDesktopWidget dw;
    ui->left->setMaximum( dw.width() );
    ui->top->setMaximum( dw.height() );
    ui->width->setMaximum( dw.width() );
    ui->height->setMaximum( dw.height() );
    // init defaults..
    if ( !m_settings->contains( "rect/left" ) ) {
        ui->left->setValue( (dw.width() - DEFAULT_WIDTH) / 2 );
        ui->top->setValue( (dw.height() - DEFAULT_HEIGHT) / 2 );
        ui->width->setValue( DEFAULT_WIDTH );
        ui->height->setValue( DEFAULT_HEIGHT );
    }
    // ..or reload previous values
    else {
        ui->left->setValue( m_settings->value( "rect/left" ).toInt() );
        ui->top->setValue( m_settings->value( "rect/top" ).toInt() );
        ui->width->setValue( m_settings->value( "rect/width" ).toInt() );
        ui->height->setValue( m_settings->value( "rect/height" ).toInt() );
        ui->frequency->setValue( m_settings->value( "rect/frequency" ).toInt() );
        ui->onTop->setChecked( m_settings->value( "rect/ontop" ).toBool() );
        ui->hBlocks->setValue( m_settings->value( "algo/hblocks" ).toInt() );
        ui->vBlocks->setValue( m_settings->value( "algo/vblocks" ).toInt() );
        if ( m_settings->contains( "algo/valid" ) )
            ui->valid->setValue( m_settings->value( "algo/valid" ).toInt() );
        ui->sensitivity->setValue( m_settings->value( "algo/sensitivity" ).toInt() );
        ui->highlight->setChecked( m_settings->value( "algo/highlight" ).toBool() );
        if ( m_settings->contains( "algo/apBest" ) )
            ui->apBest->setChecked( m_settings->value( "algo/apBest" ).toBool() );
    }
    connect( ui->left, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->top, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->width, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->height, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->frequency, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->display0, SIGNAL(toggled(bool)), this, SLOT(slotOffToggled()) );
    connect( ui->onTop, SIGNAL(toggled(bool)), this, SLOT(slotOnTopChanged()) );
    connect( ui->hBlocks, SIGNAL(valueChanged(int)), this, SLOT(slotRecParamsChanged()) );
    connect( ui->vBlocks, SIGNAL(valueChanged(int)), this, SLOT(slotRecParamsChanged()) );
    slotOnTopChanged();

    // create and train the classifier
    m_classifier = new Classifier( QSize( 30, 30 ), this );
    for ( int i = 0; i < 7; i++ ) {
        m_classifier->addClass( i, QImage( QString( ":/data/class%1.png" ).arg( i ), "PNG" ) );
        if ( QFile::exists( QString( ":/data/bomb%1.png" ).arg( i ) ) )
            m_classifier->addClass( i, QImage( QString( ":/data/bomb%1.png" ).arg( i ), "PNG" ) );
    }

    // create the recognizer
    m_recognizer = new Recognizer( m_classifier, this );
    slotRecParamsChanged();

    // create the hinter
    m_hinter = new SimpleHinter( this );

    // create the capture
    m_capture = new Capture( this );
    connect( m_capture, SIGNAL(gotPixmap(const QPixmap &, const QPoint &)),
             this, SLOT(slotProcessPixmap(const QPixmap &, const QPoint &)) );
    slotOffToggled();
    slotCapParamsChanged();
}

AppWindow::~AppWindow()
{
    saveSettings();
    delete m_settings;
    delete m_hinter;
    delete m_recognizer;
    delete m_capture;
    delete m_classifier;
    delete ui;
}

void AppWindow::saveSettings()
{
    m_settings->setValue( "rect/left", ui->left->value() );
    m_settings->setValue( "rect/top", ui->top->value() );
    m_settings->setValue( "rect/width", ui->width->value() );
    m_settings->setValue( "rect/height", ui->height->value() );
    m_settings->setValue( "rect/frequency", ui->frequency->value() );
    m_settings->setValue( "rect/ontop", ui->onTop->isChecked() );
    m_settings->setValue( "algo/hblocks", ui->hBlocks->value() );
    m_settings->setValue( "algo/vblocks", ui->vBlocks->value() );
    m_settings->setValue( "algo/valid", ui->valid->value() );
    m_settings->setValue( "algo/sensitivity", ui->sensitivity->value() );
    m_settings->setValue( "algo/highlight", ui->highlight->isChecked() );
    m_settings->setValue( "algo/apBest", ui->apBest->isChecked() );
}

void AppWindow::slotOnTopChanged()
{
    Qt::WindowFlags flags = windowFlags();
    if ( ui->onTop->isChecked() )
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags( flags );
    show();
}

void AppWindow::slotOffToggled()
{
    m_capture->setEnabled( !ui->display0->isChecked() );
}

void AppWindow::slotCapParamsChanged()
{
    QRect captureRect( ui->left->value(), ui->top->value(), ui->width->value(), ui->height->value() );
    int adjW = 0; //captureRect.width() % 30;
    int adjH = 0; //captureRect.height() % 30;
    m_capture->setGeometry( captureRect.adjusted( 0, 0, adjW, adjH ) );
    m_capture->setFrequency( ui->frequency->value() );
}

void AppWindow::slotRecParamsChanged()
{
    m_recognizer->setup( ui->hBlocks->value(), ui->vBlocks->value() );
}

#if defined(Q_WS_X11)

// use XInput for queuing a click event
#include <QX11Info>
#include <X11/extensions/XTest.h>
void leftClick()
{
    XTestFakeButtonEvent( QX11Info::display(), 1, true, CurrentTime );
}

#elif defined(Q_WS_WIN)

#include <windows.h>

void leftClick()
{
    INPUT Input={0};

    // left down
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    ::SendInput( 1, &Input, sizeof(INPUT) );

    // left up
    ::ZeroMemory( &Input, sizeof(INPUT) );
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput( 1, &Input, sizeof(INPUT) );
}

#else

#warning leftClick not implemented for this Windowing System

#endif

void AppWindow::slotProcessPixmap( const QPixmap & pixmap, const QPoint & cursor )
{
    if ( ui->display0->isChecked() )
        return;

    // 1. show original image
    ui->visualizer->setMinimumSize( pixmap.size() );
    ui->visualizer->setPixmapCursorPos( cursor );
    if ( ui->display1->isChecked() ) {
        ui->visualizer->setPixmap( pixmap );
        return;
    }

    // 2. recognize image
    bool displayRec = ui->display2->isChecked();
    float sensitivity = (float)ui->sensitivity->value() / 100.0;
    RecoResult rr = m_recognizer->recognize( pixmap, sensitivity, displayRec );
    if ( displayRec ) {
        ui->visualizer->setPixmap( m_recognizer->output() );
        return;
    }

    // bail out if too invalid
    if ( rr.invalid >= ui->valid->value() ) {
        ui->visualizer->setPixmap( m_invalidPixmap );
        return;
    }

    // 3. find hints and show result
    bool process = ui->display3->isChecked() | ui->display4->isChecked();
    if ( !process )
        return;

    HintResults hr = m_hinter->process( rr, pixmap, ui->highlight->isChecked() );
    ui->visualizer->setPixmap( m_hinter->output() );

    // 4. autoplay, if asked
    if ( hr.isEmpty() || !ui->display4->isChecked() )
        return;

    /* AUTOPLAY logic:
       - use LAST MOVE if only half of the previous (= 1 click) was performed
       - use the BEST MOVE if selected and perform 1 click
       - or choose a RANDOM MOVE and perform 2 clicks
    */

    // finish LAST MOVE if begun...
    if ( !m_lastHint.mouseFrom.isNull() ) {
        QCursor::setPos( m_lastHint.mouseTo + m_capture->geometry().topLeft() );
        leftClick();
        m_lastHint.mouseFrom = QPoint();
    }
    // ...or perform a NEW MOVE
    else {

        // perform the BEST MOVE (half)
        if ( ui->apBest->isChecked() || hr.size() == 1 ) {
            HintResult r = hr.first();
            QCursor::setPos( r.mouseFrom + m_capture->geometry().topLeft() );
            leftClick();
            m_lastHint = r;
        }

        // perform a RANDOM MOVE (complete)
        else {
            HintResult r = hr[ qrand() % hr.size() ];
            QCursor::setPos( r.mouseFrom + m_capture->geometry().topLeft() );
            leftClick();
            QCursor::setPos( r.mouseTo + m_capture->geometry().topLeft() );
            leftClick();
        }
    }
}
