/****************************************************************************
**
**  Чтение/запись wav файлов
**
****************************************************************************/

#ifndef WAVFILEIO_H
#define WAVFILEIO_H

#include <QObject>
#include <QFile>
#include <QAudioFormat>
#include <QAudioBuffer>

struct chunk
{
  char        id[4];  // chunkId
  quint32     size;   // chunkSize
};

struct RIFFHeader
{
  chunk       descriptor;     // "RIFF"   (chunkId + chunkSize = 8байт)
  char        type[4];        // "WAVE"   (format = 4 байта)
};

struct WAVEHeader
{
  chunk       descriptor;     // "fmt"  (subchunk1Id + subchunk1Size = 8 байт)
  quint16     audioFormat;    // (audioFormat = 2 байта)
  quint16     numChannels;    // (numChannels = 2 байта)
  quint32     sampleRate;     // (sampleRate = 4 байта)
  quint32     byteRate;       // (byteRate = 4 байта)
  quint16     blockAlign;     // (blockAlign = 2 байта)
  quint16     bitsPerSample;  // (bitsPerSample = 2 байта)
};

struct DATAHeader
{
  chunk       descriptor;     // "data"  (subchunk2Id + subchunk2Size = 8 байт)
};

struct CombinedHeader
{
  RIFFHeader  riff;   // (12 байт)
  WAVEHeader  wave;   // (24 байта)
  DATAHeader  data;   // (8 байт)
};
static const int HeaderLength = sizeof(CombinedHeader);

// Чтение из файла
class WavFileReader : public QFile
{
  Q_OBJECT
public:
  WavFileReader(QObject *parent = 0);

  using QFile::open;
  bool open(const QString &fileName);
  const QAudioFormat &audioFormat() const;
  qint64 headerLength() const;

private:
  bool readHeader();

private:
  QAudioFormat  _format;
  qint64        _headerLength;
};

// Запись в файл
class WaveFileWriter : public QObject
{
  Q_OBJECT
public:
  explicit WaveFileWriter(QObject *parent = 0);
  ~WaveFileWriter();

  bool open(const QString &fileName, const QAudioFormat &format);
  bool write(const QAudioBuffer &buffer);
  bool write(const QByteArray &buffer);
  bool close();
  bool isOpen() const { return file.isOpen(); }

private:
  bool writeHeader(const QAudioFormat &format);
  bool writeDataLength();

  QFile file;
  QAudioFormat _format;
  qint64 m_dataLength;
};

#endif // WAVFILEIO_H
