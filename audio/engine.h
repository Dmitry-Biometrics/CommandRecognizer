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

#ifndef ENGINE_H
#define ENGINE_H

#include "wavfileio.h"

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QObject>
#include <QVector>

QT_BEGIN_NAMESPACE
class QAudioInput;
class QAudioOutput;
QT_END_NAMESPACE


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Waveform window size in microseconds
const qint64 WaveformWindowDuration = 500 * 1000;

// Length of waveform tiles in bytes
// Ideally, these would match the QAudio*::bufferSize(), but that isn't
// available until some time after QAudio*::start() has been called, and we
// need this value in order to initialize the waveform display.
// We therefore just choose a sensible value.
const int   WaveformTileLength      = 4096;

// Fudge factor used to calculate the spectrum bar heights
const qreal SpectrumAnalyserMultiplier = 0.15;

// Disable message timeout
const int   NullMessageTimeout      = -1;

/**
 * This class interfaces with the Qt Multimedia audio classes, and also with
 * the SpectrumAnalyser class.  Its role is to manage the capture and playback
 * of audio data, meanwhile performing real-time analysis of the audio level.
 */
class Engine : public QObject
{
    Q_OBJECT

public:
    explicit Engine(QObject *parent = nullptr);
    ~Engine();

    const QList<QAudioDeviceInfo> &availableAudioInputDevices() const
                                    { return _availableAudioInputDevices; }

    const QList<QAudioDeviceInfo> &availableAudioOutputDevices() const
                                    { return _availableAudioOutputDevices; }

    QAudio::Mode mode() const { return _mode; }
    QAudio::State state() const { return _state; }

    /**
     * @brief Получить заданные параметры аудио
     * @return
     * @return Current audio format
     * @note   May be QAudioFormat() if engine is not initialized
     */
    const QAudioFormat& format() const { return _format; }

    /**
     * Stop any ongoing recording or playback, and reset to ground state.
     */
    void reset();

    /**
     * Initialize for recording
     */
    bool initializeRecord();

    /**
     * Position of the audio input device.
     * \return Position in bytes.
     */
    qint64 recordPosition() const { return _recordPosition; }

    /**
     * RMS level of the most recently processed set of audio samples.
     * \return Level in range (0.0, 1.0)
     */
    qreal rmsLevel() const { return _rmsLevel; }

    /**
     * Peak level of the most recently processed set of audio samples.
     * \return Level in range (0.0, 1.0)
     */
    qreal peakLevel() const { return _peakLevel; }

    /**
     * Position of the audio output device.
     * \return Position in bytes.
     */
    qint64 playPosition() const { return _playPosition; }

    /**
     * Length of the internal engine buffer.
     * \return Buffer length in bytes.
     */
    qint64 maxBufferLength() const;

    /**
     * Amount of data held in the buffer.
     * \return Data length in bytes.
     */
    qint64 dataLength() const { return _dataLength; }

    /**
     * @brief Установить параметры аудио
     * @param format [in] параметры аудио
     */
    bool setAudioFormat(const QAudioFormat &format);

    /**
     * @brief Установить аудио данные
     * @param buffer - [вх] данные, записываемые в _buffer
     */
    void setBuffer(const QByteArray &buffer);

#ifdef DUMP_CAPTURED_AUDIO
    void dumpData(const QString &name, const QByteArray &image);
#endif

public slots:
    void startRecording();    
    void startPlayback();
    void suspend();
    void stop();
    void setAudioInputDevice(const QAudioDeviceInfo &device);
    void setAudioOutputDevice(const QAudioDeviceInfo &device);

signals:
    void stateChanged(QAudio::Mode mode, QAudio::State state);

    /**
     * Informational message for non-modal display
     */
    void infoMessage(const QString &message, int durationMs);

    /**
     * Error message for modal display
     */
    void errorMessage(const QString &heading, const QString &detail);

    /**
     * Format of audio data has changed
     */
    void formatChanged(const QAudioFormat &format);

    /**
     * Length of buffer has changed.
     * \param duration Duration in microseconds
     */
    void bufferLengthChanged(qint64 duration);

    /**
     * Amount of data in buffer has changed.
     * \param Length of data in bytes
     */
    void dataLengthChanged(qint64 duration);

    /**
     * Position of the audio input device has changed.
     * \param position Position in bytes
     */
    void recordPositionChanged(qint64 position);

    /**
     * Position of the audio output device has changed.
     * \param position Position in bytes
     */
    void playPositionChanged(qint64 position);

    /**
     * @brief Уровень громкости изменился
     * @param rmsLevel  [in] громкость в диапазоне от 0.0 до 1.0
     * @param peakLevel [in] пиковая громкость в диапазоне от 0.0 до 1.0
     */
    void levelChanged(qreal rmsLevel, qreal peakLevel);

    /**
     * @brief Buffer containing audio data has changed.
     * @param length
     * @param buffer
     */
    void bufferChanged(qint64 length, const QByteArray &buffer);

    /**
     * @brief completeRecord
     */
    void completeRecord(const QByteArray &buffer);

private slots:
    void audioNotify();
    void audioStateChanged(QAudio::State state);
    void audioDataReady();

private:
    void resetAudioDevices();
    bool initialize();
    bool selectFormat();
    /**
     * @brief Завершить запись
     * @param flag [вх] true - завершить запись и послать сигнал о завершении (используется для добавления примера в список);
     *                  false - завершить запись, сигнал о завершении не посылать (используется, если добавлять пример в список не нужно)
     */
    void stopRecording(bool flag = false);
    void stopPlayback();
    void setState(QAudio::State state);
    void setState(QAudio::Mode mode, QAudio::State state);
    void setRecordPosition(qint64 position, bool forceEmit = false);
    void setPlayPosition(qint64 position, bool forceEmit = false);
    /**
     * @brief Вычислить уровень громкости на кадре
     * @param position [in] начало кадра
     * @param length   [in] ширина кадра
     */
    void calculateLevel(qint64 position, qint64 length);
    /**
     * @brief Обновить индикатор уровня громкости
     * @param rmsLevel  [in] громкость в диапазоне от 0.0 до 1.0
     * @param peakLevel [in] пиковая громкость в диапазоне от 0.0 до 1.0
     */
    void setLevel(qreal rmsLevel, qreal peakLevel);

#ifdef DUMP_CAPTURED_AUDIO
    /**
     * @brief Создать папку для записи выходных данных
     */
    void createOutputDir();
    QString outputPath() const { return _outputDir.path(); }
    void dumpData();
#endif


private:
    QAudio::Mode        _mode;      // режим аудио Запись/Воспроизведение
    QAudio::State       _state;     // состояние аудио-устройств
    QAudioFormat        _format;    // параметры аудио

    const QList<QAudioDeviceInfo> _availableAudioInputDevices;    // доступные устройства записи
    QAudioDeviceInfo    _audioInputDevice;                        // выбранное устройство записи
    QAudioInput*        _audioInput;                              // интерфейс взаимодействия с устройством записи
    QIODevice*          _audioInputIODevice;
    qint64              _recordPosition;                          // позиция записи

    const QList<QAudioDeviceInfo> _availableAudioOutputDevices;   // доступные устройства воспроизведения
    QAudioDeviceInfo    _audioOutputDevice;                       // выбранное устройство воспроизведения
    QAudioOutput*       _audioOutput;                             // интерфейс взаимодействия с устройством воспроизведения
    qint64              _playPosition;                            // позиция воспроизведения
    QBuffer             _audioOutputIODevice;

    QByteArray          _buffer;                                  // блок аудио-данных, полученных от устройства записи
    qint64              _maxBufferLength;                         // максимально возможный размер блока под записываемые данные (определяет максимальную длину записываемой фразы)
    qint64              _dataLength;                              // размер реально записанных данных

    int                 _levelBufferLength;                       // ширина кадра на котором рассчитывается уровень громкости
    qreal               _rmsLevel;                                // текущая громкость (от 0.0 до 1.0)
    qreal               _peakLevel;                               // пиковая громкость (от 0.0 до 1.0)

#ifdef DUMP_CAPTURED_AUDIO
    QDir                _outputDir;
#endif

};

#endif // ENGINE_H
