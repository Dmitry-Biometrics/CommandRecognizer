#include <limits>
#include <QIODevice>
#include <QTimer>
#include <QCoreApplication>
#include "AudioFormat.h"
#include "VoiceSplitter.h"

class VoiceSplitterPrivate
{
public:
    // минимальная длина фрагмента (слова) в мс
    static const quint32 FRAGMENT_MIN_LENGTH_MS = 400;

    // "запас" тишины до слова в мс
    static const quint32 FRAGMENT_MARGIN_BEFORE_MS = 400;

    // "запас" тишины после слова в мс
    static const quint32 FRAGMENT_MARGIN_AFTER_MS = 400;

    // максимальная продолжительность "тишины" внутри фрагмента
    static const quint32 FRAGMENT_MAX_SILENCE_LENGTH_MS = 400;

    // уровень при котором звук будет восприниматься как тишина, %
    static const quint32 SILENCE_MAX_VALUE = 10;

    // длительность тишины, при которой будет происходить очистка буфера от переполнения, мс
    static const quint32 SILENCE_MAX_LENGTH_MS = 2000;


public:
    VoiceSplitterPrivate(const AudioFormat& format_):
        format(format_),
        self(NULL),
        begin(NULL),
        peakStart(-1),
        index(0),
        lastImpulse(0),
        samples(0),
        totalReaded(0),
        gstart(0),
        maxSilenceValue(AudioFormat::maxValue * SILENCE_MAX_VALUE / 100),
        minFragmentLength(format_.samplesInMilliseconds(FRAGMENT_MIN_LENGTH_MS)),
        marginBefore(format_.samplesInMilliseconds(FRAGMENT_MARGIN_BEFORE_MS)),
        marginAfter(format_.samplesInMilliseconds(FRAGMENT_MARGIN_AFTER_MS)),
        maxFragmentSilenceLength(format_.samplesInMilliseconds(FRAGMENT_MAX_SILENCE_LENGTH_MS)),
        maxSilenceLength(format.samplesInMilliseconds(SILENCE_MAX_LENGTH_MS))
    {
        buff.reserve(maxSilenceLength * AudioFormat::sampleSize);
    }

    inline void addBlock(const QByteArray& readed)
    {
        buff.append(readed);
        totalReaded += readed.size();

        begin = reinterpret_cast<const AudioFormat::sampleType*>(buff.constData());
        samples = buff.size() / AudioFormat::sampleSize;

        for (; index < samples; ++index)
        {
            if (qAbs(begin[index]) > maxSilenceValue)
            {
                if (peakStart == -1)
                {
                    // начало фрагмента
                    peakStart = index;
                }

                lastImpulse = 0;
            }

            if (peakStart != -1)
            {
                ++lastImpulse;

                // конец фрагмента
                if (lastImpulse > maxFragmentSilenceLength)
                {
                    // конец фрагмента с отступом
                    int end = qMin(index - lastImpulse + marginAfter, samples);

                    // длина фрагмента > минимальной
                    if ((index - peakStart - lastImpulse) >= minFragmentLength)
                    {
                        // начало фрагмента с отступом
                        int start = qMax(peakStart - marginBefore, 0);

                        emit self->voiceFragment(buff.mid(start * AudioFormat::sampleSize,
                                                          (end - start) * AudioFormat::sampleSize));
                    }

                    gstart += end;
                    buff.remove(0, end * AudioFormat::sampleSize);
                    begin = reinterpret_cast<const AudioFormat::sampleType*>(buff.constData());
                    index = -1;

                    samples = buff.size() / AudioFormat::sampleSize;

                    peakStart = -1;
                }
            }
            else
            {
                // тишина с начала буфера
                if (index > maxSilenceLength)
                {
                    // очистка буфера
                    gstart += index;
                    buff.remove(0, index * AudioFormat::sampleSize);
                    begin = reinterpret_cast<const AudioFormat::sampleType*>(buff.constData());
                    index = -1;

                    samples = buff.size() / AudioFormat::sampleSize;
                }
            }
        }
    }

public:
    AudioFormat format;
    VoiceSplitter* self;

    const AudioFormat::sampleType* begin;
    int peakStart;
    int index;
    int lastImpulse;
    int samples;
    QByteArray buff;
    qint64 totalReaded;
    qint64 gstart;

    const AudioFormat::sampleType maxSilenceValue; // максимальное значение тишины для данного типа семпла
    const int minFragmentLength; // минимальный размер фрагмента
    const int marginBefore; // запас тишины до фрагмента
    const int marginAfter; // запас тишины после фрагмента
    const int maxFragmentSilenceLength; // максимальная продолжительность тишины в фрагменте
    const int maxSilenceLength; // максимальная длительность тишины
};

VoiceSplitter::VoiceSplitter(const AudioFormat& format):
    d_ptr(new VoiceSplitterPrivate(format))
{
    d_ptr->self = this;
}

VoiceSplitter::~VoiceSplitter()
{
    delete d_ptr;
}

void VoiceSplitter::addBlock(const QByteArray& block)
{
    d_ptr->addBlock(block);
}
