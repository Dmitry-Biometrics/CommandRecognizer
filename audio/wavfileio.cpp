/****************************************************************************
**
** Чтение/запись wav файлов
**
****************************************************************************/

#include <qendian.h>
#include <QVector>
#include <QDebug>
#include "utils.h"
#include "wavfileio.h"

// WavFileReader

WavFileReader::WavFileReader(QObject *parent)
    : QFile(parent)
    , _headerLength(0)
{

}

bool WavFileReader::open(const QString &fileName) {
    close();
    setFileName(fileName);
    return (QFile::open(QIODevice::ReadOnly) && readHeader());
}

const QAudioFormat &WavFileReader::audioFormat() const {
    return _format;
}

qint64 WavFileReader::headerLength() const {
  return _headerLength;
}

bool WavFileReader::readHeader() {
    seek(0);
    CombinedHeader header;
    bool result = read(reinterpret_cast<char *>(&header), sizeof(CombinedHeader)) == sizeof(CombinedHeader);
    if (result) {
        if ((memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0
            || memcmp(&header.riff.descriptor.id, "RIFX", 4) == 0)
            && memcmp(&header.riff.type, "WAVE", 4) == 0
            && memcmp(&header.wave.descriptor.id, "fmt ", 4) == 0
            && (header.wave.audioFormat == 1 || header.wave.audioFormat == 0)) {

            // Read off remaining header information
            if (qFromLittleEndian<quint32>(header.wave.descriptor.size) > sizeof(WAVEHeader)) {
                // Extended data available
                quint16 extraFormatBytes;
                if (peek((char*)&extraFormatBytes, sizeof(quint16)) != sizeof(quint16))
                    return false;
                const qint64 throwAwayBytes = sizeof(quint16) + qFromLittleEndian<quint16>(extraFormatBytes);
                if (read(throwAwayBytes).size() != throwAwayBytes)
                    return false;
            }

            // Establish format
            if (memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0)
                _format.setByteOrder(QAudioFormat::LittleEndian);
            else
                _format.setByteOrder(QAudioFormat::BigEndian);

            int bps = qFromLittleEndian<quint16>(header.wave.bitsPerSample);
            _format.setChannelCount(qFromLittleEndian<quint16>(header.wave.numChannels));
            _format.setCodec("audio/pcm");
            _format.setSampleRate(qFromLittleEndian<quint32>(header.wave.sampleRate));
            _format.setSampleSize(qFromLittleEndian<quint16>(header.wave.bitsPerSample));
            _format.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
        } else {
            result = false;
        }
    }
    _headerLength = pos();
    qDebug() << "read"
             << header.riff.descriptor.id << header.riff.descriptor.size << header.riff.type
             << header.wave.descriptor.id << header.wave.descriptor.size << header.wave.audioFormat << header.wave.numChannels << header.wave.sampleRate << header.wave.byteRate << header.wave.blockAlign << header.wave.bitsPerSample
             << header.data.descriptor.id << header.data.descriptor.size;
    qDebug() << "read" << _format;
    return result;
}

// WaveFileWriter
WaveFileWriter::WaveFileWriter(QObject *parent)
    : QObject(parent)
    , m_dataLength(0)
{
}

WaveFileWriter::~WaveFileWriter()
{
    close();
}

bool WaveFileWriter::open(const QString& fileName, const QAudioFormat& format)
{
  qDebug() << "write" << format;
    if (file.isOpen())
        return false; // file already open

    if (format.codec() != "audio/pcm" || format.sampleType() != QAudioFormat::SignedInt)
        return false; // data format is not supported

    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false; // unable to open file for writing

    if (!writeHeader(format))
        return false;

    _format = format;
    return true;
}

bool WaveFileWriter::write(const QAudioBuffer &buffer)
{
    if (buffer.format() != _format)
        return false; // buffer format has changed

    qint64 written = file.write((const char *)buffer.constData(), buffer.byteCount());
    m_dataLength += written;
    return written == buffer.byteCount();
}
bool WaveFileWriter::write(const QByteArray &buffer) {
  QAudioBuffer buf(buffer, _format);
  return write(buf);
}

bool WaveFileWriter::close()
{
    bool result = false;
    if (file.isOpen()) {
        Q_ASSERT(m_dataLength < INT_MAX);
        result = writeDataLength();

        m_dataLength = 0;
        file.close();
    }
    return result;
}

bool WaveFileWriter::writeHeader(const QAudioFormat &format)
{
    // check if format is supported
    if (format.byteOrder() == QAudioFormat::BigEndian || format.sampleType() != QAudioFormat::SignedInt)
        return false;

    CombinedHeader header;
    memset(&header, 0, HeaderLength);

#ifndef Q_LITTLE_ENDIAN
    // only implemented for LITTLE ENDIAN
    return false;
#else
    // RIFF header
    memcpy(header.riff.descriptor.id, "RIFF", 4);
    header.riff.descriptor.size = 0; // this will be updated with correct duration:
                                     // m_dataLength + HeaderLength - 8
    // WAVE header
    memcpy(header.riff.type, "WAVE", 4);
    memcpy(header.wave.descriptor.id, "fmt ", 4);
    header.wave.descriptor.size = quint32(16);
    header.wave.audioFormat = quint16(1);
    header.wave.numChannels = quint16(format.channelCount());
    header.wave.sampleRate = quint32(format.sampleRate());
    header.wave.byteRate = quint32(format.sampleRate() * format.channelCount() * format.sampleSize() / 8);
    header.wave.blockAlign = quint16(format.channelCount() * format.sampleSize() / 8);
    header.wave.bitsPerSample = quint16(format.sampleSize());

    // DATA header
   memcpy(header.data.descriptor.id,"data", 4);
    header.data.descriptor.size = 0; // this will be updated with correct data length: m_dataLength

    qDebug() << "write"
             << header.riff.descriptor.id << header.riff.descriptor.size << header.riff.type
             << header.wave.descriptor.id << header.wave.descriptor.size << header.wave.audioFormat << header.wave.numChannels << header.wave.sampleRate << header.wave.byteRate
             << header.wave.blockAlign << header.wave.bitsPerSample
             << header.data.descriptor.id << header.data.descriptor.size;

    return (file.write(reinterpret_cast<const char *>(&header), HeaderLength) == HeaderLength);
#endif
}

bool WaveFileWriter::writeDataLength()
{
#ifndef Q_LITTLE_ENDIAN
    // only implemented for LITTLE ENDIAN
    return false;
#endif

    if (file.isSequential())
        return false;

    // seek to RIFF header size, see header.riff.descriptor.size above
    if (!file.seek(4))
        return false;

    quint32 length = m_dataLength + HeaderLength - 8;
    if (file.write(reinterpret_cast<const char *>(&length), 4) != 4)
        return false;

    // seek to DATA header size, see header.data.descriptor.size above
    if (!file.seek(40))
        return false;

    return file.write(reinterpret_cast<const char *>(&m_dataLength), 4) == 4;
}
