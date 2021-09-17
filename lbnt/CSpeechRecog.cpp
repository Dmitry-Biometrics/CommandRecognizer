#include "CSpeechRecog.h"

// Конструктор
CSpeechRecog::CSpeechRecog(const QString &pathHmm, const QString &pathLm,
                           const QString &pathDict, const QString &pathGram, QObject *parent) :
    QObject(parent),
    _thread(nullptr),
    _config(nullptr),
    _ps(nullptr),
    _pathHmm(pathHmm),
    _pathLm(pathLm),
    _pathDict(pathDict),
    _pathGram(pathGram),
    _sampleRate(8000)
{

#ifdef __linux__
    if (_pathLm.isEmpty() && _pathHmm.isEmpty() && _pathDict.isEmpty()) {
        _pathLm = LMPATH;
        _pathHmm = HMMDIR;
        _pathDict = DICTPATH;
    }
#endif

}

CSpeechRecog::~CSpeechRecog()
{
    this->free();
    if (_thread) {
        if (_thread->isRunning()){
            _thread->wait();
        }
        delete _thread;
        _thread = nullptr;
    }
}

// Инициализировать
void CSpeechRecog::init()
{
    if (_thread) {
        if (_thread->isRunning()){
            _thread->wait();
        }
        delete _thread;
        _thread = nullptr;
    }
    _thread = new InitThread(this);
    _thread->start();

}

// Проверить инициализирован ли
bool CSpeechRecog::isInit() const
{
    if (_config != nullptr && _ps != nullptr)
        return true;
    else return false;
}

// Сбросить
void CSpeechRecog::free()
{
    if (_ps != nullptr) ps_free(_ps);
    _ps = nullptr;
    if (_config != nullptr) cmd_ln_free_r(_config);
   _config = nullptr;
}

// Установить языковую модель
void CSpeechRecog::setLM(const QString &path){

    _pathLm = path;
//    updateModel();
}

// Установить акустическую модель
void CSpeechRecog::setHmm(const QString &path){
    _pathHmm = path;
//    updateModel();
}

// Установить словарь
void CSpeechRecog::setDict(const QString &path){
    _pathDict = path;
    //    updateModel();
}

// Установить грамматику
void CSpeechRecog::setGram(const QString &path)
{
    _pathGram = path;
}

// Установить частоту дискретизации
void CSpeechRecog::setSampleRate(int samplerate)
{
    _sampleRate = samplerate;
}

// Получить языковую модель
QString CSpeechRecog::getLM() const
{
    return _pathLm;
}

// Получить акустическую модель
QString CSpeechRecog::getHmm() const
{
    return _pathHmm;
}

// Получить словарь
QString CSpeechRecog::getDict() const
{
    return _pathDict;
}

// Получить грамматику
QString CSpeechRecog::getGram() const
{
    return _pathGram;
}

// Получить частоту дискретизации
int CSpeechRecog::getSampleRate() const
{
    return _sampleRate;
}

// Считать звук из ByteArray
void CSpeechRecog::readBA(const QByteArray &ba, ps_decoder_t *ps) const
{

    int rv = ps_start_utt(ps);
    if (rv < 0) runtime_error("Failed to start utt, see log for details");

    QDataStream stream(ba);
    int16 buff[512];

    int counter = 0;
    int counter1 = 0;
    while (!stream.atEnd()){
        int nsamp = stream.readRawData((char*)buff,sizeof(int16)*512);
        if (!(nsamp%2)) {
            rv = ps_process_raw(ps, buff, nsamp/2, FALSE, FALSE);
        }
        else {
            rv = ps_process_raw(ps, buff, (nsamp-1)/2, FALSE, FALSE);
        }
        counter++;
        counter1 += nsamp;
    }

//    rv = ps_process_raw(ps, (const int16*) ba.constData(), ba.size()/2, FALSE, TRUE);


    rv = ps_end_utt(ps);

}

// Считать звук из файла
void CSpeechRecog::readFile(const QString &path, ps_decoder_t *ps) const
{
    FILE *fh = nullptr;
    fh = fopen(path.toLocal8Bit().data(), "rb");

    if (!fh) throw runtime_error("Unable to open input file");

    int rv = ps_start_utt(ps);
    if (rv < 0) runtime_error("Failed to start utt, see log for details");

    int16 buff[512];
    while (!feof(fh)) {
        size_t nsamp;
        nsamp = fread(buff, 2, 512, fh);
        rv = ps_process_raw(ps, buff, nsamp, FALSE, FALSE);
    }

    rv = ps_end_utt(ps);

    fclose(fh);

}

// Обновить языковую модель
void CSpeechRecog::updateModel(){
    free();
    init();
}

// Декодировать данные
void CSpeechRecog::decode(ps_decoder_t *ps, QString &str, int &score) const
{
    const char *hyp = ps_get_hyp(ps, &score);
    str.append(hyp);
}

// Декодировать raw
void CSpeechRecog::decodeRaw(const QByteArray &raw, QString &str, int &score) const
{
    if (!raw.isEmpty() && isInit()) {
        readBA(raw,_ps);
        decode(_ps,str, score);
    }
}

// Декодировать wav
void CSpeechRecog::decodeWav(const QByteArray &wav, QString &str, int &score) const
{
    if (!wav.isEmpty() && isInit()) {
        // Смещение 44 байта звголовка
        char *data =  (char *)wav.data();
        data += 44;
        QByteArray raw(data, wav.size()-44);
        decodeRaw(raw,str,score);
    }

}

// Преобразовать фразу формата raw в текст
QString CSpeechRecog::rawToString(const QByteArray &raw) const
{
    if (raw.isEmpty()) return QString();
    if (!isInit()) return QString();
    readBA(raw,_ps);
    QString str;
    int score = 0;
    decode(_ps,str, score);
    return str;
}

// Преобразовать фразу в текст
QString CSpeechRecog::rawToString(const QString &path) const
{
    if (!isInit()) return QString();
    QString str;
    readFile(path,_ps);
    int score = 0;
    decode(_ps,str, score);
    return str;
}

// Преобразовать wav в строку
QString CSpeechRecog::wavToString(const QString &path) const
{
    if (!isInit()) return QString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw runtime_error("Unable to open input file");
    QByteArray wav = file.readAll();
    file.close();
    int score = 0;
    QString str;
    decodeWav(wav,str,score);
    return str;
}

void CSpeechRecog::InitThread::run()
{
    try {
        if (_self->_pathGram.isEmpty())
            _self->_config = cmd_ln_init(nullptr, ps_args(), TRUE,
                                     "-hmm", _self->_pathHmm.toLocal8Bit().data(),
                                     "-lm", _self->_pathLm.toLocal8Bit().data(),
                                     "-dict", _self->_pathDict.toLocal8Bit().data(),
                                     "-samprate",  (QString("%1").arg(_self->_sampleRate)).toLocal8Bit().data(),
                                     nullptr);
        else
            _self->_config = cmd_ln_init(nullptr, ps_args(), TRUE,
                                         "-hmm", _self->_pathHmm.toLocal8Bit().data(),
                                         /*"-lm", _self->_pathLm.toLocal8Bit().data(),*/
                                         "-dict", _self->_pathDict.toLocal8Bit().data(),
                                          "-jsgf", _self->_pathGram.toLocal8Bit().data(),
                                         "-samprate",  (QString("%1").arg(_self->_sampleRate)).toLocal8Bit().data(),
                                         nullptr);

        if (!_self->_config) throw runtime_error("Failed to create config object, see log for details");

        _self->_ps = ps_init(_self->_config);

        if (!_self->_ps) throw runtime_error("Failed to create recognizer, see log for details");
    } catch (std::runtime_error err) {
        emit _self->initError(QString(err.what()));
        return;
    }
    emit _self->initFinished();
}
