/**
  * \author Уткин Д.К.
  *
  * Аудио формат
  */

#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

#include <QtGlobal>

//! аудио формат
struct AudioFormat
{
  typedef qint16 sampleType; //!< типа семпла
  static const int sampleSize = sizeof(sampleType); //!< размер семпла
  static const sampleType maxValue; //!< максимальное значение
  static const sampleType minValue; //!< минимальное значение

  qint8 channels; //!< количество каналов. 0 - чтобы не использовать устройство
  quint16 samplingRate; //!< частота дискретизации в Hz

  AudioFormat(qint8 channels_ = 1, quint16 samplingRate_ = 8000):
    channels(channels_), samplingRate(samplingRate_)
  {
  }

  inline quint32 bytesInMilliseconds(quint32 ms) const
  {
    return (ms * sampleSize * channels * samplingRate) / 1000;
  }

  inline quint32 millisecondsInBytes(quint32 bytes) const
  {
    return bytes / bytesInMilliseconds(1);
  }

  inline quint32 samplesInMilliseconds(quint32 ms) const
  {
    return (ms * channels * samplingRate) / 1000;
  }

  inline quint32 millisecondsInSamples(quint32 samples) const
  {
      return samples / samplesInMilliseconds(1);
  }
};


#endif // AUDIOFORMAT_H
