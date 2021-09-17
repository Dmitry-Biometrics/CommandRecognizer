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

#include "engine.h"
#include "utils.h"
#include "wavefilewriter.h"
#include <math.h>
#include <QAudioInput>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMetaObject>
#include <QSet>
#include <QThread>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Минимальная длина записываемой фразы (в байтах)
const int WaveformMinDataLength   = 20000;

const qint64 BufferDurationUs       = 15 * 1000000; // 15 секунд. Определяет максимальную длину записываемой фразы
const int    NotifyIntervalMs       = 100;

// Size of the level calculation window in microseconds
const int    LevelWindowUs          = 0.1 * 1000000;

//-----------------------------------------------------------------------------
// Constructor and destructor
//-----------------------------------------------------------------------------

Engine::Engine(QObject *parent)
  :   QObject(parent)
  ,   _mode(QAudio::AudioInput)
  ,   _state(QAudio::StoppedState)
  ,   _availableAudioInputDevices
      (QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
  ,   _audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
  ,   _audioInput(nullptr)
  ,   _audioInputIODevice(nullptr)
  ,   _recordPosition(0)
  ,   _availableAudioOutputDevices
      (QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
  ,   _audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
  ,   _audioOutput(nullptr)
  ,   _playPosition(0)
  ,   _maxBufferLength(0)
  ,   _dataLength(0)
  ,   _levelBufferLength(0)
  ,   _rmsLevel(0.0)
  ,   _peakLevel(0.0)
{
//  initialize();

#ifdef DUMP_DATA
  createOutputDir();
#endif
}

Engine::~Engine()
{

}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

bool Engine::initializeRecord()
{
//  ENGINE_DEBUG << "Engine::initializeRecord";
//  reset();
  return initialize();
}

qint64 Engine::maxBufferLength() const
{
  return _maxBufferLength;
}

// Установить параметры аудио
bool Engine::setAudioFormat(const QAudioFormat &format)
{
  const bool changed = (format != _format);
  if(!_audioInputDevice.isFormatSupported(format)) return false;
  _format = format;
  _levelBufferLength = audioLength(_format, LevelWindowUs);
//  ENGINE_DEBUG << "Engine::setAudioFormat _levelBufferLength" << _levelBufferLength;
  if (changed)
    emit formatChanged(_format);
  return true;
}

// Установить аудио данные
void Engine::setBuffer(const QByteArray &buffer) {
  ENGINE_DEBUG << "Engine::setBuffer" << _dataLength;
  _buffer = buffer;
  _dataLength = _buffer.size();
  emit bufferChanged(_dataLength, _buffer);
}

//-----------------------------------------------------------------------------
// Public slots
//-----------------------------------------------------------------------------

void Engine::startRecording()
{
  if (!_audioInput) { return; }
  if (QAudio::AudioInput == _mode &&
      QAudio::SuspendedState == _state) {
    _audioInput->resume();
  } else {
     _buffer.resize(_maxBufferLength);
    _buffer.fill(0);
    setRecordPosition(0, true);
    stopPlayback();
    _mode = QAudio::AudioInput;
    connect(_audioInput, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(audioStateChanged(QAudio::State)));
    connect(_audioInput, SIGNAL(notify()),
            this, SLOT(audioNotify()));
    _dataLength = 0;
    emit dataLengthChanged(0);
    _audioInputIODevice = _audioInput->start();
    connect(_audioInputIODevice, SIGNAL(readyRead()),
            this, SLOT(audioDataReady()));
  }
}

void Engine::startPlayback()
{
  if (!_audioOutput) return;

  if (QAudio::AudioOutput == _mode &&
      QAudio::SuspendedState == _state) {
#ifdef Q_OS_WIN
    // The Windows backend seems to internally go back into ActiveState
    // while still returning SuspendedState, so to ensure that it doesn't
    // ignore the resume() call, we first re-suspend
    _audioOutput->suspend();
#endif
    _audioOutput->resume();
  } else {
    setPlayPosition(0, true);
    stopRecording();
    _mode = QAudio::AudioOutput;
    connect(_audioOutput, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(audioStateChanged(QAudio::State)));
    connect(_audioOutput, SIGNAL(notify()),
            this, SLOT(audioNotify()));
    _audioOutputIODevice.close();
    _audioOutputIODevice.setBuffer(&_buffer);
    _audioOutputIODevice.open(QIODevice::ReadOnly);
    _audioOutput->start(&_audioOutputIODevice);
  }

}

void Engine::suspend()
{
  if (QAudio::ActiveState == _state ||
      QAudio::IdleState == _state) {
    switch (_mode) {
    case QAudio::AudioInput:
      _audioInput->suspend();
      break;
    case QAudio::AudioOutput:
      _audioOutput->suspend();
      break;
    }
  }
}
void Engine::stop()
{
  if (QAudio::ActiveState == _state ||
      QAudio::IdleState == _state) {
    switch (_mode) {
    case QAudio::AudioInput:
      stopRecording(true);
      break;
    case QAudio::AudioOutput:
      stopPlayback();
      break;
    }
  }
}

void Engine::setAudioInputDevice(const QAudioDeviceInfo &device)
{
  if (device.deviceName() != _audioInputDevice.deviceName()) {
    _audioInputDevice = device;
    initialize();
  }
}

void Engine::setAudioOutputDevice(const QAudioDeviceInfo &device)
{
  if (device.deviceName() != _audioOutputDevice.deviceName()) {
    _audioOutputDevice = device;
    initialize();
  }
}


//-----------------------------------------------------------------------------
// Private slots
//-----------------------------------------------------------------------------

void Engine::audioNotify()
{
  switch (_mode) {
  case QAudio::AudioInput: {
//    ENGINE_DEBUG << "Engine::audioNotify" << "QAudio::AudioInput";
    const qint64 recordPosition = qMin(_maxBufferLength, audioLength(_format, _audioInput->processedUSecs()));
    setRecordPosition(recordPosition);
    const qint64 levelPosition = _dataLength - _levelBufferLength;
    if (levelPosition >= 0)
      calculateLevel(levelPosition, _levelBufferLength);
    emit bufferChanged(_dataLength, _buffer);
  }
    break;
  case QAudio::AudioOutput: {
    const qint64 playPosition = audioLength(_format, _audioOutput->processedUSecs());
    setPlayPosition(qMin(maxBufferLength(), playPosition));
    const qint64 levelPosition = playPosition - _levelBufferLength;
    if ( playPosition >= _dataLength ) stopPlayback();
    if ( levelPosition >= 0 && (levelPosition + _levelBufferLength < _dataLength) )
      calculateLevel(levelPosition, _levelBufferLength);
  }
    break;
  }
}

void Engine::audioStateChanged(QAudio::State state)
{
//  ENGINE_DEBUG << "Engine::audioStateChanged from" << _state << "to" << state;
  if (QAudio::StoppedState == state) {
    // Check error
    QAudio::Error error = QAudio::NoError;
    switch (_mode) {
    case QAudio::AudioInput:
      error = _audioInput->error();
      break;
    case QAudio::AudioOutput:
      error = _audioOutput->error();
      break;
    }
    if (QAudio::NoError != error) {
      reset();
      return;
    }
  }
  setState(state);
}

void Engine::audioDataReady()
{
  const qint64 bytesReady = _audioInput->bytesReady();
  const qint64 bytesSpace = _buffer.size() - _dataLength;
  const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

  const qint64 bytesRead = _audioInputIODevice->read(
        _buffer.data() + _dataLength,
        bytesToRead);

  if (bytesRead) {
    _dataLength += bytesRead;
    emit dataLengthChanged(dataLength());
  }

  if (_buffer.size() == _dataLength) {
    stopRecording(true);
  }
}

//-----------------------------------------------------------------------------
// Private functions
//-----------------------------------------------------------------------------

void Engine::resetAudioDevices()
{
  delete _audioInput;
  _audioInput = nullptr;
  _audioInputIODevice = nullptr;
  setRecordPosition(0);
  delete _audioOutput;
  _audioOutput = nullptr;
  setPlayPosition(0);
  setLevel(0.0, 0.0);
}

void Engine::reset()
{
  stopRecording();
  stopPlayback();
  setState(QAudio::AudioInput, QAudio::StoppedState);
  setAudioFormat(QAudioFormat());
  _buffer.clear();
  _maxBufferLength = 0;
  _dataLength = 0;
  _levelBufferLength = 0;
  emit dataLengthChanged(0);
  resetAudioDevices();
}

bool Engine::initialize()
{
  bool result = false;

  QAudioFormat format = _format;

//  ENGINE_DEBUG << "______________-Engine::initialize" << "format" << _format;

  if (selectFormat()) {
    if (_format != format) {
      resetAudioDevices();
      _maxBufferLength = audioLength(_format, BufferDurationUs);
      _buffer.resize(_maxBufferLength);
      _buffer.fill(0);
      emit bufferLengthChanged(maxBufferLength());
      emit bufferChanged(0, _buffer);
      _audioInput = new QAudioInput(_audioInputDevice, _format, this);
      _audioInput->setNotifyInterval(NotifyIntervalMs);
      result = true;
      _audioOutput = new QAudioOutput(_audioOutputDevice, _format, this);
      _audioOutput->setNotifyInterval(NotifyIntervalMs);
    }
  } else {
    //      emit errorMessage(tr("Audio format not supported"), formatToString(_format));
    emit errorMessage(tr("No common input / output format found"), "");
  }

//  ENGINE_DEBUG << "Engine::initialize" << "_maxBufferLength" << _maxBufferLength;
//  ENGINE_DEBUG << "Engine::initialize" << "_dataLength" << _dataLength;
//  ENGINE_DEBUG << "Engine::initialize" << "format" << _format;

  return result;
}

bool Engine::selectFormat()
{
  bool foundSupportedFormat = false;

  if (QAudioFormat() != _format) {
    QAudioFormat format = _format;
    if (_audioOutputDevice.isFormatSupported(format)) {
      setAudioFormat(format);
      foundSupportedFormat = true;
    }
  } else {
    QList<int> sampleRatesList;
#ifdef Q_OS_WIN
    // The Windows audio backend does not correctly report format support
    // (see QTBUG-9100).  Furthermore, although the audio subsystem captures
    // at 11025Hz, the resulting audio is corrupted.
    sampleRatesList += 8000;
#endif

    sampleRatesList += _audioInputDevice.supportedSampleRates();
    sampleRatesList += _audioOutputDevice.supportedSampleRates();
    sampleRatesList = sampleRatesList.toSet().toList(); // remove duplicates
    qSort(sampleRatesList);
//    ENGINE_DEBUG << "Engine::initialize frequenciesList" << sampleRatesList;

    QList<int> channelsList;
    channelsList += _audioInputDevice.supportedChannelCounts();
    channelsList += _audioOutputDevice.supportedChannelCounts();
    channelsList = channelsList.toSet().toList();
    qSort(channelsList);
//    ENGINE_DEBUG << "Engine::initialize channelsList" << channelsList;

    QAudioFormat format;
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
//    format.setSampleRate(44100); //!!!
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    int sampleRate, channels;
    foreach (sampleRate, sampleRatesList) {
      if (foundSupportedFormat)
        break;
      format.setSampleRate(sampleRate);
      foreach (channels, channelsList) {
        format.setChannelCount(channels);
        const bool inputSupport  = _audioInputDevice.isFormatSupported(format);
        const bool outputSupport = _audioOutputDevice.isFormatSupported(format);
//        ENGINE_DEBUG << "Engine::initialize checking " << format
//                     << "input" << inputSupport
//                     << "output" << outputSupport;
        if (inputSupport && outputSupport) {
          foundSupportedFormat = true;
          break;
        }
      }
    }

    if (!foundSupportedFormat)
      format = QAudioFormat();

    setAudioFormat(format);
  }

  return foundSupportedFormat;
}



void Engine::stopRecording(bool flag)
{


  if (_audioInput) {
    _audioInput->stop();
    QCoreApplication::instance()->processEvents();
    _audioInput->disconnect();
    setRecordPosition(0);
  }
  _audioInputIODevice = nullptr;


  //TODO убрать константу (повесить её на кнопку Стоп, чтобы запрещалось нажимать её раньше времени)
  if (_dataLength > WaveformMinDataLength && QAudio::AudioInput == _mode && flag) {
    QByteArray buf(_buffer.data(), _dataLength);
    ENGINE_DEBUG << "Engine::stopRecording()" << _buffer.size() << _maxBufferLength << _dataLength << buf.size();
    emit completeRecord(buf);
  }

#ifdef DUMP_CAPTURED_AUDIO
  dumpData();
#endif
}

void Engine::stopPlayback()
{
//  ENGINE_DEBUG << "Engine::stopPlayback()";
  if (_audioOutput) {
    _audioOutput->stop();
    QCoreApplication::instance()->processEvents();
    _audioOutput->disconnect();
    setPlayPosition(0);
  }
}

void Engine::setState(QAudio::State state)
{
  const bool changed = (_state != state);
  _state = state;
  if (changed)
    emit stateChanged(_mode, _state);
}

void Engine::setState(QAudio::Mode mode, QAudio::State state)
{
  const bool changed = (_mode != mode || _state != state);
  _mode = mode;
  _state = state;
  if (changed)
    emit stateChanged(_mode, _state);
}

void Engine::setRecordPosition(qint64 position, bool forceEmit)
{
  const bool changed = (_recordPosition != position);
  _recordPosition = position;
  if (changed || forceEmit)
    emit recordPositionChanged(_recordPosition);
}

void Engine::setPlayPosition(qint64 position, bool forceEmit)
{
  const bool changed = (_playPosition != position);
  _playPosition = position;
  if (changed || forceEmit)
    emit playPositionChanged(_playPosition);
}

// Вычислить уровень громкости на кадре
void Engine::calculateLevel(qint64 position, qint64 length)
{
#ifdef DISABLE_LEVEL
  Q_UNUSED(position)
  Q_UNUSED(length)
#else
  Q_ASSERT(position + length <= _dataLength);

  qreal peakLevel = 0.0;

  qreal sum = 0.0;
  const char *ptr = _buffer.constData() + position;
  const char *const end = ptr + length;
  while (ptr < end) {
    const qint16 value = *reinterpret_cast<const qint16*>(ptr);
    const qreal fracValue = pcmToReal(value);
    peakLevel = qMax(peakLevel, fracValue);
    sum += fracValue * fracValue;
    ptr += 2;
  }
  const int numSamples = length / 2;
  qreal rmsLevel = sqrt(sum / numSamples);

  rmsLevel = qMax(qreal(0.0), rmsLevel);
  rmsLevel = qMin(qreal(1.0), rmsLevel);
  setLevel(rmsLevel, peakLevel);

//  ENGINE_DEBUG << "Engine::calculateLevel" << "pos:" << position << "len:" << length
//               << "rms:" << rmsLevel << "peak:" << peakLevel;
#endif
}

void Engine::setLevel(qreal rmsLevel, qreal peakLevel)
{
  _rmsLevel = rmsLevel;
  _peakLevel = peakLevel;
  emit levelChanged(_rmsLevel, _peakLevel);
}

#ifdef DUMP_CAPTURED_AUDIO
void Engine::createOutputDir()
{
  _outputDir.setPath("output");

  // Ensure output directory exists and is empty
  if (_outputDir.exists()) {
    const QStringList files = _outputDir.entryList(QDir::Files);
    QString file;
    foreach (file, files)
      _outputDir.remove(file);
  } else {
    QDir::current().mkdir("output");
  }
}

void Engine::dumpData()
{
  if(_dataLength == 0) return;
  const QString txtFileName = _outputDir.filePath("data.txt");
  QFile txtFile(txtFileName);
  txtFile.open(QFile::WriteOnly | QFile::Text);
  QTextStream stream(&txtFile);
  const qint16 *ptr = reinterpret_cast<const qint16*>(_buffer.constData());
  const int numSamples = _dataLength / (2 * _format.channelCount());
  for (int i=0; i<numSamples; ++i) {
    stream << i << "\t" << *ptr << "\n";
    ptr += _format.channelCount();
  }

  const QString pcmFileName = _outputDir.filePath("data.pcm");
  QFile pcmFile(pcmFileName);
  pcmFile.open(QFile::WriteOnly);
  pcmFile.write(_buffer.constData(), _dataLength);
}

void Engine::dumpData(const QString &name, const QByteArray &image)
{
  QString fileName = name + (".txt");
  const QString txtFileName = _outputDir.filePath(fileName);
  QFile txtFile(txtFileName);
  txtFile.open(QFile::WriteOnly | QFile::Text);
  QTextStream stream(&txtFile);
  const qint16 *ptr = reinterpret_cast<const qint16*>(image.constData());
  const int numSamples = image.size() / (2 * _format.channelCount());
  for (int i=0; i<numSamples; ++i) {
    stream << i << "\t" << *ptr << "\n";
    ptr += _format.channelCount();
  }
  txtFile.close();

  fileName = name + (".wav");
  const QString wavFileName = _outputDir.filePath(fileName);
  WaveFileWriter wavFile;
  if (wavFile.open(wavFileName, _format)) {
    wavFile.write(image);
    wavFile.close();
  }
}
#endif // DUMP_CAPTURED_AUDIO
