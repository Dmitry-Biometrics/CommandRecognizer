#include <QFile>
#include <QDebug>
#include <pocketsphinx.h>
#include "AudioFormat.h"
#include "VoiceRecognizer.h"

class VoiceRecognizerPrivate
{
public:
    AudioFormat format;
    ps_decoder_t* ps;
    cmd_ln_t* config;

    VoiceRecognizerPrivate():
        ps(NULL), config(NULL)
    {
    }
};


VoiceRecognizer::VoiceRecognizer():
    d_ptr(new VoiceRecognizerPrivate)
{
}

VoiceRecognizer::~VoiceRecognizer()
{
    free();
    delete d_ptr;
}

bool VoiceRecognizer::init(const AudioFormat& format, const QString& hmmPath,
                           const QString& lmPath, const QString& dictPath)
{
    if (d_ptr->config != NULL)
    {
        qDebug() << "already initialized!";
        return false;
    }

    // to stop spamming into stdout
//    err_set_logfile("/dev/null");

    d_ptr->config = cmd_ln_init(NULL, ps_args(), TRUE,
                                "-hmm", hmmPath.toStdString().c_str(),
                                "-lm", lmPath.toStdString().c_str(),
                                "-dict", dictPath.toStdString().c_str(),
                                NULL);
    if (d_ptr->config == NULL)
    {
        qDebug() << "failed to initialize config";
        return false;
    }

    d_ptr->ps = ps_init(d_ptr->config);
    if (d_ptr->ps == NULL)
    {
        d_ptr->config = NULL;
        qDebug() << "failed to initialize voice recognition";
        return false;
    }

    d_ptr->format = format;

    return true;
}

void VoiceRecognizer::free()
{
    ps_free(d_ptr->ps);
    d_ptr->ps = NULL;
    // we don't need to free config
    d_ptr->config = NULL;
}

bool VoiceRecognizer::isInit() const
{
    return d_ptr->ps != NULL;
}

bool VoiceRecognizer::recognize(const QByteArray& fragment, QString& hypothesis, int* score)
{
    int res = ps_start_utt(d_ptr->ps);
    if (res < 0)
    {
        qDebug() << "Error while recognizing data: ps_start_utt";
        return false;
    }

    res = ps_process_raw(d_ptr->ps,
                         reinterpret_cast<const AudioFormat::sampleType*>(fragment.constData()),
                         fragment.size() / AudioFormat::sampleSize, FALSE, TRUE);
    if (res < 0)
    {
        qDebug() << "Error while recognizing data: ps_process_raw";
        return false;
    }

    res = ps_end_utt(d_ptr->ps);
    if (res < 0)
    {
        qDebug() << "Error while recognizing data: ps_end_utt";
        return false;
    }


    int32 resScore = 0;
    char const* resHypothesis = NULL;
    char const* uttid = NULL;

    resHypothesis = ps_get_hyp(d_ptr->ps, &resScore);
    if (resHypothesis == NULL)
    {
        qDebug() << "Error while getting hypothesis";
        return false;
    }

    hypothesis = QString::fromUtf8(resHypothesis);

    if (score)
    {
        *score = resScore;
    }

    return true;
}
