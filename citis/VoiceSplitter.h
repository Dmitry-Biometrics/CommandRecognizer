#ifndef VOICESPLITTER_H
#define VOICESPLITTER_H

#include <QObject>

struct AudioFormat;

class VoiceSplitterPrivate;

class VoiceSplitter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VoiceSplitter)
    Q_DECLARE_PRIVATE(VoiceSplitter)

public:
    VoiceSplitter(const AudioFormat& format);
    ~VoiceSplitter();

    void addBlock(const QByteArray& block);
    
signals:
    void voiceFragment(const QByteArray& fragment);
    
private:
    VoiceSplitterPrivate* d_ptr;
};

#endif // VOICESPLITTER_H
