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
#include "SimpleHinter.h"
#include "ui_Window.h"

#include <QFile>
#include <QImage>
#include <QDesktopWidget>
#include <QSettings>

#define DEFAULT_WIDTH 252
#define DEFAULT_HEIGHT 330

Window::Window( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::WindowForm )
    , m_settings( new QSettings() )
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
        ui->sensitivity->setValue( m_settings->value( "algo/sensitivity" ).toInt() );
        ui->highlight->setChecked( m_settings->value( "algo/highlight" ).toBool() );
    }
    connect( ui->left, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->top, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->width, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->height, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
    connect( ui->frequency, SIGNAL(valueChanged(int)), this, SLOT(slotCapParamsChanged()) );
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
    slotCapParamsChanged();
}

Window::~Window()
{
    saveSettings();
    delete m_settings;
    delete m_hinter;
    delete m_recognizer;
    delete m_capture;
    delete m_classifier;
    delete ui;
}

void Window::saveSettings()
{
    m_settings->setValue( "rect/left", ui->left->value() );
    m_settings->setValue( "rect/top", ui->top->value() );
    m_settings->setValue( "rect/width", ui->width->value() );
    m_settings->setValue( "rect/height", ui->height->value() );
    m_settings->setValue( "rect/frequency", ui->frequency->value() );
    m_settings->setValue( "rect/ontop", ui->onTop->isChecked() );
    m_settings->setValue( "algo/hblocks", ui->hBlocks->value() );
    m_settings->setValue( "algo/vblocks", ui->vBlocks->value() );
    m_settings->setValue( "algo/sensitivity", ui->sensitivity->value() );
    m_settings->setValue( "algo/highlight", ui->highlight->isChecked() );
}

void Window::slotOnTopChanged()
{
    Qt::WindowFlags flags = windowFlags();
    if ( ui->onTop->isChecked() )
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags( flags );
    show();
}

void Window::slotCapParamsChanged()
{
    QRect captureRect( ui->left->value(), ui->top->value(), ui->width->value(), ui->height->value() );
    int adjW = captureRect.width() % 30;
    int adjH = captureRect.height() % 30;
    m_capture->setGeometry( captureRect.adjusted( 0, 0, adjW, adjH ) );
    m_capture->setFrequency( ui->frequency->value() );
}

void Window::slotRecParamsChanged()
{
    m_recognizer->setup( ui->hBlocks->value(), ui->vBlocks->value() );
}

void Window::slotProcessPixmap( const QPixmap & pixmap, const QPoint & cursor )
{
    // show original image
    ui->visualizer->setMinimumSize( pixmap.size() );
    ui->visualizer->setPixmapCursorPos( cursor );
    if ( ui->display1->isChecked() )
        ui->visualizer->setOriginalPixmap( pixmap );

    // process image
    bool displayRec = ui->display2->isChecked();
    float sensitivity = (float)ui->sensitivity->value() / 100.0;
    RecoResult rr = m_recognizer->recognize( pixmap, sensitivity, displayRec );
    if ( displayRec )
        ui->visualizer->setOriginalPixmap( m_recognizer->output() );

    // show results
    if ( ui->display3->isChecked() ) {
        bool highlight = ui->highlight->isChecked();
        m_hinter->process( rr, pixmap, highlight );
        ui->visualizer->setOriginalPixmap( m_hinter->output() );
    }
}
