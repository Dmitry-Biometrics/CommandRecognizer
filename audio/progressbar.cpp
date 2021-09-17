/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "progressbar.h"
#include <QPainter>
#include <QDebug>

ProgressBar::ProgressBar(QWidget *parent)
    :   QWidget(parent)
    ,   m_bufferLength(0)
    ,   m_recordPosition(0)
    ,   m_playPosition(0)
    ,   m_windowPosition(0)
    ,   m_windowLength(0)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(30);
#ifdef SUPERIMPOSE_PROGRESS_ON_WAVEFORM
    setAutoFillBackground(false);
#endif
}

ProgressBar::~ProgressBar()
{

}

void ProgressBar::reset()
{
    m_bufferLength = 0;
    m_recordPosition = 0;
    m_playPosition = 0;
    m_windowPosition = 0;
    m_windowLength = 0;
    update();
}

void ProgressBar::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    QColor bufferColor(0, 0, 255);
    QColor windowColor(0, 255, 0);

#ifdef SUPERIMPOSE_PROGRESS_ON_WAVEFORM
    bufferColor.setAlphaF(0.5);
    windowColor.setAlphaF(0.5);
#else
    painter.fillRect(rect(), Qt::black);
#endif

//вот тут разница
    if (!m_bufferLength) return;
    QRect bar = rect();
    const qreal record = qreal(m_playPosition+m_recordPosition) / m_bufferLength;
    bar.setRight(rect().left() + record * rect().width());
    painter.fillRect(bar, bufferColor);

    // рисование окна просто для красоты
    QRect window = rect();
    window.setLeft(bar.right());
    const qreal windowWidth = qreal(1000) / m_bufferLength;
    window.setWidth(windowWidth * rect().width());
    painter.fillRect(window, windowColor);


//    //вот тут разница
//        if (!m_bufferLength) return;
//        QRect bar = rect();
//        const qreal play = qreal(m_playPosition) / m_bufferLength;
//        qDebug() << "ProgressBar::paintEvent bar1" << bar <<  play << m_playPosition << m_bufferLength;
//        bar.setLeft(rect().left() + play * rect().width());
//        qDebug() << "ProgressBar::paintEvent bar2" << bar << rect();

//        const qreal record = qreal(m_recordPosition) / m_bufferLength;
//        qDebug() << "ProgressBar::paintEvent bar3" << record << m_recordPosition;
//        bar.setRight(rect().left() + record * rect().width());
//        qDebug() << "ProgressBar::paintEvent bar4" << bar << rect();
//        painter.fillRect(bar, bufferColor);

//        // рисование окна на котором вычисляется спектр
//    //    QRect window = rect();
//    //    const qreal windowLeft = qreal(m_windowPosition) / m_bufferLength;
//    //    window.setLeft(rect().left() + windowLeft * rect().width()); // рисуем окно на котором показывается спектр
//    //    const qreal windowWidth = qreal(m_windowLength) / m_bufferLength;
//    //    window.setWidth(windowWidth * rect().width());
//    //    painter.fillRect(window, windowColor);

//        // рисование окна просто для красоты
//        QRect window = rect();
//        qDebug() << "ProgressBar::paintEvent1" << window << bar;
//        window.setLeft(bar.right());
//        qDebug() << "ProgressBar::paintEvent2" << window << bar;
//        const qreal windowWidth = qreal(1000) / m_bufferLength;
//        window.setWidth(windowWidth * rect().width());
//        qDebug() << "ProgressBar::paintEvent2" << window << windowWidth;
//        painter.fillRect(window, windowColor);
}

void ProgressBar::bufferLengthChanged(qint64 bufferSize)
{
    m_bufferLength = bufferSize;
    m_recordPosition = 0;
    m_playPosition = 0;
    m_windowPosition = 0;
    m_windowLength = 0;
    repaint();
}

void ProgressBar::recordPositionChanged(qint64 recordPosition)
{
    Q_ASSERT(recordPosition >= 0);
    Q_ASSERT(recordPosition <= m_bufferLength);
    m_recordPosition = recordPosition;
    repaint();
}

void ProgressBar::playPositionChanged(qint64 playPosition)
{
    Q_ASSERT(playPosition >= 0);
    Q_ASSERT(playPosition <= m_bufferLength);
    m_playPosition = playPosition;
    repaint();
}

void ProgressBar::windowChanged(qint64 position, qint64 length)
{
    Q_ASSERT(position >= 0);
    Q_ASSERT(position <= m_bufferLength);
    Q_ASSERT(position + length <= m_bufferLength);
    m_windowPosition = position;
    m_windowLength = length;
    repaint();
}
