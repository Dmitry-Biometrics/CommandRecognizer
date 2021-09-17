/**
 * @author  Максим Секретов
 * @brief
 * @file    MainWindow.h
 * @date    2016.05
 * @version 1.0
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "audio/engine.h"
#include <QTimer>
#include <QTextStream>
#include "citis/VoiceSplitter.h"
#include "citis/AudioFormat.h"
#include "lbnt/CSpeechRecog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected slots:
    void startRecord();
    void stopRecord();
    void completeRecord(const QByteArray &record);
    void voiceFragment(const QByteArray &fragment);
    void bufferChanged(qint64 length, const QByteArray &buffer);
    void msgError(const QString &err);

protected:
    bool initAudio();
    bool initSpeechRecognizer();
    void writeTxt(const QString &path, const QString &txt);

private:
    Ui::MainWindow *ui;
    Engine _engine;
    QTimer _timer;
    VoiceSplitter *_voiceSplitter;
    AudioFormat _audioFormat;
    CSpeechRecog  *_speech;
    QDataStream _stream;
    QFile _file;
    int _counterFragment;
    int _counterBlock;
    QByteArray _buffer;
};

#endif // MAINWINDOW_H
