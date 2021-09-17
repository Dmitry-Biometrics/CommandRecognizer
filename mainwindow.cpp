#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#define TIMEOUT_VALUE 2000

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  _voiceSplitter = new VoiceSplitter(_audioFormat);
  // Русская модель
  QString pathHmm(QString(QCoreApplication::applicationDirPath()).append("/model2/2000"));
  QString pathLM(QString(QCoreApplication::applicationDirPath()).append("/model2/ru.lm"));
  QString pathDict(QString(QCoreApplication::applicationDirPath()).append("/model2/ru.dic"));
  QString pathGram(QString(QCoreApplication::applicationDirPath()).append("/model2/zitic.jsgf"));
  _speech = new CSpeechRecog(pathHmm, pathLM, pathDict, pathGram, this);
  _speech->setSampleRate(_audioFormat.samplingRate);

  ui->label_2->setText(QString("<font size=20 color=#FF0000><b>%1</b></font>").arg("Инициализация"));
  if (!initAudio()) msgError("Ошибка инициализации записи");
  else initSpeechRecognizer();
}

MainWindow::~MainWindow()
{
  delete ui;
  //    _timer.stop();
  _engine.stop();
  //    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(stopRecord()));
  disconnect(_voiceSplitter, SIGNAL(voiceFragment(QByteArray)), this, SLOT(voiceFragment(QByteArray)));
  disconnect(&_engine, SIGNAL(completeRecord(QByteArray)), this, SLOT(completeRecord(QByteArray)));
  disconnect(&_engine, SIGNAL(bufferChanged(qint64,QByteArray)), this, SLOT(bufferChanged(qint64,QByteArray)));
  delete _voiceSplitter;
  delete _speech;
}

bool MainWindow::initAudio()
{
  QList<QAudioDeviceInfo> listAudioInputInfo = _engine.availableAudioInputDevices();

  QList<QAudioDeviceInfo> listAudioOutputInfo = _engine.availableAudioOutputDevices();

  qDebug() << "Available audio input devices:";
  foreach (QAudioDeviceInfo device, listAudioInputInfo) {
    qDebug() << device.deviceName();
  }

  qDebug() << "Available audio output devices:";
  foreach (QAudioDeviceInfo device, listAudioOutputInfo) {
    qDebug() << device.deviceName();
  }

  // Select default input device
  QAudioDeviceInfo device = QAudioDeviceInfo::defaultInputDevice();

  qDebug() << "Default audio input devices:";
  qDebug() << device.deviceName();

  qDebug() << "Codecs:";
  QStringList listCodecs = device.supportedCodecs();
  foreach (QString codec, listCodecs) {
    qDebug() << codec;
  }

  qDebug() << "Sample rate:";
  QList<int> listSampleRates = device.supportedSampleRates();
  foreach (int sampleRate, listSampleRates) {
    qDebug() << sampleRate;
  }

  qDebug() << "Sample size:";
  QList<int> listSampleSizes = device.supportedSampleSizes();
  foreach (int sampleSize, listSampleSizes) {
    qDebug() << sampleSize;
  }

  if (!_engine.initializeRecord()) return false;

  QAudioFormat audioFormat;
  audioFormat.setByteOrder(QAudioFormat::LittleEndian);
  audioFormat.setCodec(listCodecs[0]);
  audioFormat.setSampleSize(16);
  audioFormat.setSampleType(QAudioFormat::SignedInt);
  audioFormat.setSampleRate(8000);
  audioFormat.setChannelCount(1);

  qDebug() << "Audio format: " << audioFormat;

  connect(&_engine, SIGNAL(completeRecord(QByteArray)), this, SLOT(completeRecord(QByteArray)));
  connect(&_engine, SIGNAL(bufferChanged(qint64,QByteArray)), this, SLOT(bufferChanged(qint64,QByteArray)));

  //    connect(&_timer, SIGNAL(timeout()), this, SLOT(stopRecord()));
  connect(_voiceSplitter, SIGNAL(voiceFragment(QByteArray)), this, SLOT(voiceFragment(QByteArray)));

  _engine.setAudioInputDevice(device);
  if (device.isFormatSupported(audioFormat)) {
    _engine.setAudioFormat(audioFormat);
  } else
    return false;

  return true;
}

bool MainWindow::initSpeechRecognizer()
{
  connect(_speech, SIGNAL(initFinished()), this, SLOT(startRecord()));
  connect(_speech, SIGNAL(initError(QString)), this, SLOT(msgError(QString)));
  _speech->init();
  return true;
}

void MainWindow::completeRecord(const QByteArray &record)
{
  _engine.startRecording();
}

void MainWindow::voiceFragment(const QByteArray &fragment)
{
  QString str1;
  str1 = _speech->rawToString(fragment);
  _engine.dumpData(QString("test/%1.wav").arg(_counterFragment),fragment);
  writeTxt(QString("test/%1.txt").arg(_counterFragment), str1);
  ui->label->setText(QString("<font size=16 color=#000000><b>%1</b></font>").arg(str1));
  _counterFragment++;
}

void MainWindow::bufferChanged(qint64 length, const QByteArray &buffer)
{
  QByteArray ba(buffer.constData(),length);
  //    qDebug() << "Length: " << ba.length();
  QByteArray block = ba.remove(0,_buffer.length());
  _voiceSplitter->addBlock(block);
  _counterBlock++;
  //    qDebug() << "Add Block " << _counterBlock << " size " << block.size();
  _buffer = QByteArray(buffer.constData(),length);

}

void MainWindow::startRecord()
{
  _engine.startRecording();
  _counterFragment = 0;
  _counterBlock = 0;
  ui->label_2->setText(QString("<font size=20 color=#FF0000><b>%1</b></font>").arg("Слушаю"));
}

void MainWindow::stopRecord()
{
  _engine.stop();
}

void MainWindow::writeTxt(const QString &path, const QString &txt)
{
  QTextStream stream;
  QFile file(path);
  if (file.open(QIODevice::WriteOnly)){
    stream.setDevice(&file);
    stream << txt;
    file.flush();
    file.close();
  }
}

void MainWindow::msgError(const QString &err)
{
  ui->label_2->setText(QString("<font size=20 color=#FF0000><b>%1</b></font>").arg(err));
}
