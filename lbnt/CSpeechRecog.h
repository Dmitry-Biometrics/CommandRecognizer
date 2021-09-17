/**
 * @author  Калашников Дмитрий, Максим Секретов
 * @brief   Компонет преобразования речевой фразы в текст с помощью средств CMUSphinx
 * @file    CSpeechRecog.h
 * @date    2016.05
 * @version 2.0
 */

#ifndef CSPEECHRECOG_H
#define CSPEECHRECOG_H

#include <QObject>
#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>
#include <QDataStream>
#include <QFile>
#include <QThread>
#include <pocketsphinx.h>

#ifdef __linux__
#define MODELDIR "/usr/local/share/pocketsphinx/model"
#define HMMDIR (MODELDIR "/en-us/en-us")
#define LMPATH (MODELDIR "/en-us/en-us.lm.bin")
#define DICTPATH (MODELDIR "/en-us/cmudict-en-us.dict")
#endif

using namespace std;

class CSpeechRecog : public QObject
{
    Q_OBJECT
public:

    // Конструктор
    explicit CSpeechRecog(const QString &pathHmm, const QString &pathLm, const QString &pathDict, const QString &pathGram = QString(), QObject *parent = 0);
    ~CSpeechRecog();

    // Инициализировать
    void init();
    // Проверить инициализирован ли
    bool isInit() const;
    // Сбросить
    void free();
    // Обновить модель
    void updateModel();
    // Декодировать raw
    void decodeRaw(const QByteArray &raw, QString &str, int &score) const;
    // Декодировать wav
    void decodeWav(const QByteArray &wav, QString &str, int &score) const;
    // Преобразовать фразу формата raw в текст
    QString rawToString(const QByteArray &raw) const;
    // Преобразовать фразу формата raw в строку
    QString rawToString(const QString &path) const;
    // Преобразовать wav в строку
    QString wavToString(const QString &path) const;
    // Установить языковую модель
    void setLM(const QString &path);
    // Установить акустическую модель
    void setHmm(const QString &path);
    // Установить словарь
    void setDict(const QString &path);
    // Установить грамматику
    void setGram(const QString &path);
    // Установить частоту дискретизации
    void setSampleRate(int samplerate);
    // Получить языковую модель
    QString getLM() const;
    // Получить акустическую модель
    QString getHmm() const;
    // Получить словарь
    QString getDict() const;
    // Получить грамматику
    QString getGram() const;
    // Получить частоту дискретизации
    int getSampleRate() const;

signals:
    void initError(const QString &err);
    void initFinished();

protected:

    // Класс для инициализации в потоке
    class InitThread: public QThread
    {
    public:
        InitThread(CSpeechRecog *speech) : _self(speech) {}
        void run();
    protected:
        CSpeechRecog *_self;
    };

    // Считать звук из ByteArray
    void readBA(const QByteArray &ba, ps_decoder_t *ps) const;
    // Считать звук из файла
    void readFile(const QString &path, ps_decoder_t *ps) const;
    // Декодировать данные
    void decode(ps_decoder_t *ps, QString &str, int &score) const;

private:
    InitThread *_thread;
    cmd_ln_t *_config;  // Конфигурация
    ps_decoder_t *_ps;  // Декодер CMUSpinx
    QString _pathHmm;   // Путь к папке акустической модели
    QString _pathLm;    // Путь к файлу языковой модели
    QString _pathDict;  // Путь файлу словаря
    QString _pathGram;  // Путь к файлу грамматики
    int _sampleRate;
};

#endif // CSPEECHRECOG_H
