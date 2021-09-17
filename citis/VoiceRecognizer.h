#ifndef VOICERECOGNIZER_H
#define VOICERECOGNIZER_H

#include <QThread>

struct AudioFormat;

class VoiceRecognizerPrivate;

class VoiceRecognizer: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VoiceRecognizer)
    Q_DECLARE_PRIVATE(VoiceRecognizer)

public:
    VoiceRecognizer();
    ~VoiceRecognizer();

    bool init(const AudioFormat& format, const QString& hmmPath,
              const QString& lmPath, const QString& dictPath);
    void free();

    bool isInit() const;

    bool recognize(const QByteArray& fragment, QString& hypothesis, int* score = NULL);

private:
    VoiceRecognizerPrivate* d_ptr;
};

#endif // VOICERECOGNIZER_H
