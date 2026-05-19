#include "sioworker.h"

#include "aspeqtsettings.h"
#include <QFile>
#include <QDateTime>
#include <QVector>
#include <QtDebug>
#include <cmath>

static quint32 casChunkMagic(const QByteArray &header)
{
    return (quint8)header.at(0)
            + (quint8)header.at(1) * 256
            + (quint8)header.at(2) * 65536
            + (quint8)header.at(3) * 16777216;
}

static quint16 casChunkLength(const QByteArray &header)
{
    return (quint8)header.at(4) + (quint8)header.at(5) * 256;
}

static quint16 casChunkAux(const QByteArray &header)
{
    return (quint8)header.at(6) + (quint8)header.at(7) * 256;
}

static QString casChunkName(quint32 magic)
{
    QByteArray chunkName(4, '\0');
    chunkName[0] = magic & 0xFF;
    chunkName[1] = (magic >> 8) & 0xFF;
    chunkName[2] = (magic >> 16) & 0xFF;
    chunkName[3] = (magic >> 24) & 0xFF;
    for (int i = 0; i < chunkName.size(); ++i) {
        char c = chunkName.at(i);
        if (c < 32 || c > 126) {
            chunkName[i] = '?';
        }
    }
    return QString::fromLatin1(chunkName);
}

struct FskSignalRun
{
    int signal;
    double lengthSeconds;
};

struct FskSignalSegment
{
    int signal;
    double startSeconds;
    double endSeconds;
};

struct FskRecognizeSignal
{
    double startSeconds;
    double lengthSeconds;
};

static constexpr double FskMinIrgLengthSeconds = 0.05;
static constexpr double FskBitDeviation = 0.25;
static constexpr double FskBitMiddle = 0.5;
static constexpr double FskBlockHeaderDeviation = 0.5;
static constexpr int FskBlockHeaderSignalCount = 20;

static void normalizeFskSignalRuns(QVector<FskSignalRun> &signalRuns)
{
    for (int i = 1; i < signalRuns.size();) {
        if (signalRuns.at(i - 1).signal == signalRuns.at(i).signal) {
            signalRuns[i - 1].lengthSeconds += signalRuns.at(i).lengthSeconds;
            signalRuns.remove(i);
        } else {
            ++i;
        }
    }
}

static void mergeShortFskSignals(QVector<FskSignalRun> &signalRuns, double minLengthSeconds)
{
    if (signalRuns.size() < 2) {
        return;
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < signalRuns.size(); ++i) {
            if (signalRuns.at(i).lengthSeconds >= minLengthSeconds) {
                continue;
            }

            if (signalRuns.size() == 1) {
                return;
            }

            if (i == 0) {
                signalRuns[1].lengthSeconds += signalRuns.at(0).lengthSeconds;
                signalRuns.remove(0);
            } else if (i == signalRuns.size() - 1) {
                signalRuns[i - 1].lengthSeconds += signalRuns.at(i).lengthSeconds;
                signalRuns.remove(i);
            } else {
                signalRuns[i - 1].lengthSeconds += signalRuns.at(i).lengthSeconds + signalRuns.at(i + 1).lengthSeconds;
                signalRuns.remove(i, 2);
            }

            normalizeFskSignalRuns(signalRuns);
            changed = true;
            break;
        }
    }
}

static QVector<FskSignalSegment> buildFskSignalSegments(const QVector<FskSignalRun> &signalRuns)
{
    QVector<FskSignalSegment> segments;
    segments.reserve(signalRuns.size());

    double position = 0.0;
    for (const FskSignalRun &run : signalRuns) {
        FskSignalSegment segment;
        segment.signal = run.signal;
        segment.startSeconds = position;
        segment.endSeconds = position + run.lengthSeconds;
        segments.append(segment);
        position = segment.endSeconds;
    }

    return segments;
}

static void mergeRecognizeSignals(QVector<FskRecognizeSignal> &recognizeRuns, int pos)
{
    if (pos < 1 || pos + 1 >= recognizeRuns.size()) {
        return;
    }

    recognizeRuns[pos - 1].lengthSeconds += recognizeRuns[pos].lengthSeconds + recognizeRuns[pos + 1].lengthSeconds;
    recognizeRuns.remove(pos, 2);
}

static void adjustFskBitLength(double *bitLengthSeconds, double byteWindowLengthSeconds)
{
    if (!bitLengthSeconds || *bitLengthSeconds <= 0.0) {
        return;
    }

    const double newBitLengthSeconds = byteWindowLengthSeconds / 10.0;
    const double maxBitLengthSeconds = *bitLengthSeconds * 1.1;
    const double minBitLengthSeconds = *bitLengthSeconds * 0.9;
    if (newBitLengthSeconds < maxBitLengthSeconds && newBitLengthSeconds > minBitLengthSeconds) {
        *bitLengthSeconds *= (newBitLengthSeconds / *bitLengthSeconds - 1.0) / 10.0 + 1.0;
    }
}

static bool recognizeFskByte(const QVector<double> &signalLengths, double *bitLengthSeconds,
                             int *readOffset, int *numEndMarkBits, quint8 *decodedByte)
{
    if (readOffset) {
        *readOffset = 0;
    }
    if (numEndMarkBits) {
        *numEndMarkBits = 1;
    }
    if (decodedByte) {
        *decodedByte = 0;
    }
    if (signalLengths.isEmpty() || !bitLengthSeconds || *bitLengthSeconds <= 0.0) {
        return false;
    }

    double currentBitLengthSeconds = *bitLengthSeconds;
    const double minLengthSeconds = currentBitLengthSeconds * 0.10;
    const double byteLengthSeconds = currentBitLengthSeconds * (9.0 + FskBitMiddle);
    const double minDistanceFromEdge = currentBitLengthSeconds * (0.5 - FskBitDeviation);

    QVector<FskRecognizeSignal> recognizeSignals;
    recognizeSignals.reserve(signalLengths.size());

    double copiedSeconds = 0.0;
    double positionSeconds = 0.0;
    int offset = 0;
    while (offset < signalLengths.size() && copiedSeconds < byteLengthSeconds) {
        const double signalLengthSeconds = signalLengths.at(offset);
        copiedSeconds += signalLengthSeconds;
        recognizeSignals.append({positionSeconds, signalLengthSeconds});
        positionSeconds += signalLengthSeconds;
        ++offset;
    }

    if (copiedSeconds < byteLengthSeconds) {
        return false;
    }

    if (offset & 1) {
        return false;
    }

    if (offset < 2) {
        return false;
    }

    adjustFskBitLength(&currentBitLengthSeconds, positionSeconds);
    if (offset & 1) {
        return false;
    }
    if (positionSeconds > currentBitLengthSeconds * (10.0 + 10000.0) &&
            positionSeconds < currentBitLengthSeconds * 15.0) {
        return false;
    }

    for (int i = 2; i < recognizeSignals.size() - 1;) {
        if (recognizeSignals.at(i).lengthSeconds >= minLengthSeconds) {
            ++i;
            continue;
        }

        if (recognizeSignals.at(i + 1).lengthSeconds >= minLengthSeconds || i + 1 == recognizeSignals.size() - 1) {
            mergeRecognizeSignals(recognizeSignals, i);
            continue;
        }

        double deletePosI = (recognizeSignals.at(i + 1).startSeconds + recognizeSignals.at(i + 1).lengthSeconds) / currentBitLengthSeconds;
        double deletePosI1 = recognizeSignals.at(i).startSeconds / currentBitLengthSeconds;
        deletePosI = std::fabs(deletePosI - std::round(deletePosI));
        deletePosI1 = std::fabs(deletePosI1 - std::round(deletePosI1));
        if (deletePosI < deletePosI1) {
            mergeRecognizeSignals(recognizeSignals, i);
        } else {
            mergeRecognizeSignals(recognizeSignals, i + 1);
        }
    }

    int signalIndex = 0;
    bool shortSignalIsGlitch = true;
    quint16 byte = 0;
    quint16 mask = 0x01;
    int endMarkBits = 1;

    for (int bitPos = 0; bitPos < 9; ++bitPos) {
        const double samplePositionSeconds = currentBitLengthSeconds * (FskBitMiddle + bitPos);
        if (!(recognizeSignals.at(signalIndex).startSeconds < samplePositionSeconds)) {
            return false;
        }

        while (recognizeSignals.at(signalIndex).startSeconds + recognizeSignals.at(signalIndex).lengthSeconds <= samplePositionSeconds) {
            if (shortSignalIsGlitch) {
                return false;
            }
            shortSignalIsGlitch = true;
            ++signalIndex;
            if (signalIndex >= recognizeSignals.size()) {
                return false;
            }
        }

        shortSignalIsGlitch = false;

        if (samplePositionSeconds - recognizeSignals.at(signalIndex).startSeconds < minDistanceFromEdge ||
                recognizeSignals.at(signalIndex).startSeconds + recognizeSignals.at(signalIndex).lengthSeconds - samplePositionSeconds < minDistanceFromEdge) {
            return false;
        }

        if (signalIndex & 1) {
            byte |= mask;
            ++endMarkBits;
        } else {
            endMarkBits = 1;
        }

        mask <<= 1;
    }

    byte >>= 1;

    if (readOffset) {
        *readOffset = offset;
    }
    if (numEndMarkBits) {
        *numEndMarkBits = endMarkBits;
    }
    if (decodedByte) {
        *decodedByte = quint8(byte);
    }
    *bitLengthSeconds = currentBitLengthSeconds;
    return true;
}

static bool isFskByteLengthGood(const QVector<double> &signalLengths, int signalsToCheck,
                                double *byteLengthSeconds, double *deviationSum)
{
    if (!byteLengthSeconds || !deviationSum || signalsToCheck < 4) {
        return false;
    }

    double bitLengthSeconds = *byteLengthSeconds / 10.0;
    double nextFullByteSeconds = *byteLengthSeconds;
    double accumulatedSeconds = 0.0;
    double previousFullByteSeconds = 0.0;
    int byteCount = 0;

    *deviationSum = 0.0;

    for (int i = 0; i < signalsToCheck; i += 2) {
        if (signalLengths.at(i + 1) >= FskMinIrgLengthSeconds) {
            if (byteCount == 0) {
                return false;
            }
            break;
        }

        accumulatedSeconds += signalLengths.at(i) + signalLengths.at(i + 1);
        if (accumulatedSeconds < nextFullByteSeconds - FskBlockHeaderDeviation * bitLengthSeconds) {
            continue;
        }
        if (accumulatedSeconds > nextFullByteSeconds + FskBlockHeaderDeviation * bitLengthSeconds) {
            return false;
        }

        nextFullByteSeconds = accumulatedSeconds + *byteLengthSeconds;
        previousFullByteSeconds = accumulatedSeconds;
        ++byteCount;
    }

    if (byteCount == 0) {
        return false;
    }

    *byteLengthSeconds = previousFullByteSeconds / byteCount;
    bitLengthSeconds = *byteLengthSeconds / 10.0;

    for (int i = 0; i < signalsToCheck; ++i) {
        const double signalLengthSeconds = signalLengths.at(i);
        if ((i & 1) && signalLengthSeconds >= FskMinIrgLengthSeconds) {
            return true;
        }

        const double scaledLength = signalLengthSeconds / bitLengthSeconds;
        double expectedBits = std::round(scaledLength);
        if (expectedBits == 0.0) {
            expectedBits = 1.0;
        }

        const double deviation = std::fabs(scaledLength - expectedBits);
        if (deviation > FskBlockHeaderDeviation) {
            return false;
        }
        *deviationSum += deviation;
    }

    int readOffset = 0;
    int endMarkBits = 0;
    quint8 byte = 0;
    double adjustedBitLengthSeconds = bitLengthSeconds;
    return recognizeFskByte(signalLengths, &adjustedBitLengthSeconds, &readOffset, &endMarkBits, &byte);
}

static bool detectFskBaudrate(const QVector<double> &signalLengths, double *bitLengthSeconds,
                              int *detectedBaudRate, QString *errorMessage)
{
    if (bitLengthSeconds) {
        *bitLengthSeconds = 0.0;
    }
    if (detectedBaudRate) {
        *detectedBaudRate = 0;
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    int signalsToCheck = qMin(signalLengths.size(), FskBlockHeaderSignalCount);
    signalsToCheck &= ~1;
    if (signalsToCheck < 4) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK block is too short to detect baud rate.");
        }
        return false;
    }

    const double minByteLengthSeconds = 10.0 / 1500.0;
    const double maxByteLengthSeconds = 10.0 / 300.0;

    double bestByteLengthSeconds = 0.0;
    double bestDeviation = 2.0 * signalsToCheck;
    double accumulatedSeconds = 0.0;

    for (int i = 0; i < signalsToCheck; i += 2) {
        accumulatedSeconds += signalLengths.at(i) + signalLengths.at(i + 1);
        double candidateByteLengthSeconds = accumulatedSeconds;
        if (candidateByteLengthSeconds < minByteLengthSeconds) {
            continue;
        }
        if (candidateByteLengthSeconds > maxByteLengthSeconds) {
            break;
        }

        double deviation = 0.0;
        if (!isFskByteLengthGood(signalLengths, signalsToCheck, &candidateByteLengthSeconds, &deviation)) {
            continue;
        }

        if (deviation <= bestDeviation) {
            bestByteLengthSeconds = candidateByteLengthSeconds;
            bestDeviation = deviation + 0.0001;
        }
    }

    if (bestByteLengthSeconds <= 0.0) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK decoder could not detect a standard baud rate.");
        }
        return false;
    }

    const double detectedBitLengthSeconds = bestByteLengthSeconds / 10.0;
    const int baudRate = qRound(1.0 / detectedBitLengthSeconds);

    if (bitLengthSeconds) {
        *bitLengthSeconds = detectedBitLengthSeconds;
    }
    if (detectedBaudRate) {
        *detectedBaudRate = baudRate;
    }
    return true;
}

static bool decodeFskSignalsWithDeserializer(const QVector<FskSignalRun> &signalRuns, QByteArray *decodedBytes,
                                             int *detectedBaudRate, QString *errorMessage)
{
    if (decodedBytes) {
        decodedBytes->clear();
    }
    if (detectedBaudRate) {
        *detectedBaudRate = 0;
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    if (signalRuns.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK decoder received an empty signal block.");
        }
        return false;
    }

    QVector<double> signalLengths;
    signalLengths.reserve(signalRuns.size());
    for (const FskSignalRun &run : signalRuns) {
        signalLengths.append(run.lengthSeconds);
    }

    double bitLengthSeconds = 0.0;
    int baudRate = 0;
    QByteArray bytes;
    int currentSignal = 0;

    while (!signalLengths.isEmpty()) {
        if (currentSignal == 1) {
            if (signalLengths.first() >= bitLengthSeconds * FskBitDeviation) {
                currentSignal = 0;
                signalLengths.removeFirst();
                continue;
            }
            signalLengths.removeFirst();
            currentSignal = 0;
            continue;
        }

        if (signalLengths.first() <= 0.0) {
            signalLengths.removeFirst();
            currentSignal ^= 1;
            continue;
        }

        if (!detectFskBaudrate(signalLengths, &bitLengthSeconds, &baudRate, errorMessage)) {
            break;
        }

        const double minimumByteLengthSeconds = bitLengthSeconds * (9.0 + FskBitMiddle);
        bool decodedSubBlock = false;

        while (!signalLengths.isEmpty()) {
            if (currentSignal == 1) {
                if (signalLengths.first() >= bitLengthSeconds * FskBitDeviation) {
                    break;
                }
                signalLengths.removeFirst();
                currentSignal = 0;
                continue;
            }

            if (signalLengths.first() <= 0.0) {
                signalLengths.removeFirst();
                currentSignal ^= 1;
                continue;
            }

            double totalLengthSeconds = 0.0;
            for (double lengthSeconds : signalLengths) {
                totalLengthSeconds += lengthSeconds;
            }
            if (totalLengthSeconds < minimumByteLengthSeconds) {
                break;
            }

            int readOffset = 0;
            int endMarkBits = 0;
            quint8 byte = 0;
            if (!recognizeFskByte(signalLengths, &bitLengthSeconds, &readOffset, &endMarkBits, &byte)) {
                if (!decodedSubBlock && bytes.isEmpty()) {
                    if (errorMessage && errorMessage->isEmpty()) {
                        *errorMessage = QObject::tr("FSK decoder detected %1 baud, but could not decode the first byte.")
                                .arg(baudRate);
                    }
                }
                break;
            }

            bytes.append(char(byte));
            decodedSubBlock = true;

            const int removedSignals = qMax(0, readOffset - 1);
            signalLengths.remove(0, removedSignals);
            currentSignal = (currentSignal + removedSignals) & 1;
            if (signalLengths.isEmpty()) {
                break;
            }

            signalLengths[0] -= bitLengthSeconds * endMarkBits;
            if (signalLengths[0] < bitLengthSeconds * FskBitDeviation) {
                signalLengths.removeFirst();
                currentSignal ^= 1;
            }
        }

        if (!decodedSubBlock) {
            break;
        }
    }

    if (bytes.isEmpty()) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QObject::tr("FSK decoder detected %1 baud, but did not recover any bytes.")
                    .arg(baudRate);
        }
        return false;
    }

    if (decodedBytes) {
        *decodedBytes = bytes;
    }
    if (detectedBaudRate) {
        *detectedBaudRate = baudRate;
    }
    return true;
}

static int fskSignalAtTime(const QVector<FskSignalSegment> &segments, double timeSeconds, double *distanceFromEdge = nullptr)
{
    if (segments.isEmpty()) {
        if (distanceFromEdge) {
            *distanceFromEdge = 0.0;
        }
        return 1;
    }

    for (const FskSignalSegment &segment : segments) {
        if (timeSeconds < segment.endSeconds) {
            if (distanceFromEdge) {
                *distanceFromEdge = qMin(timeSeconds - segment.startSeconds, segment.endSeconds - timeSeconds);
            }
            return segment.signal;
        }
    }

    if (distanceFromEdge) {
        *distanceFromEdge = 0.0;
    }
    return segments.constLast().signal;
}

static QByteArray decodeBytesFromFskSegments(const QVector<FskSignalSegment> &segments, double startSeconds,
                                             double bitLengthSeconds)
{
    QByteArray bytes;
    if (segments.isEmpty() || bitLengthSeconds <= 0.0) {
        return bytes;
    }

    const double totalLengthSeconds = segments.constLast().endSeconds;
    double byteStart = startSeconds;

    while (byteStart + bitLengthSeconds * 9.5 <= totalLengthSeconds) {
        int sampledBits[10];
        bool ambiguous = false;

        for (int bit = 0; bit < 10; ++bit) {
            double distanceFromEdge = 0.0;
            double sampleTime = byteStart + bitLengthSeconds * (0.5 + bit);
            sampledBits[bit] = fskSignalAtTime(segments, sampleTime, &distanceFromEdge);
            if (distanceFromEdge < bitLengthSeconds * 0.10) {
                ambiguous = true;
                break;
            }
        }

        if (ambiguous || sampledBits[0] != 0 || sampledBits[9] != 1) {
            break;
        }

        quint8 byte = 0;
        for (int bit = 0; bit < 8; ++bit) {
            if (sampledBits[bit + 1]) {
                byte |= quint8(1u << bit);
            }
        }

        bytes.append(char(byte));
        byteStart += bitLengthSeconds * 10.0;
    }

    return bytes;
}

static bool decodeFskSignalsBySampling(const QVector<FskSignalRun> &signalRuns, int nominalBaudRate,
                                       QByteArray *decodedBytes, QString *errorMessage)
{
    if (decodedBytes) {
        decodedBytes->clear();
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    if (signalRuns.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK decoder received an empty signal block.");
        }
        return false;
    }

    QVector<FskSignalRun> cleanedRuns = signalRuns;
    int nominalBaud = qBound(300, nominalBaudRate > 0 ? nominalBaudRate : 600, 1500);
    double nominalBitLengthSeconds = 1.0 / static_cast<double>(nominalBaud);
    mergeShortFskSignals(cleanedRuns, nominalBitLengthSeconds * 0.10);
    normalizeFskSignalRuns(cleanedRuns);

    QVector<FskSignalSegment> segments = buildFskSignalSegments(cleanedRuns);
    if (segments.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK block does not contain any usable signal segments.");
        }
        return false;
    }

    struct BestCandidate
    {
        QByteArray bytes;
        int baud = 0;
    } bestCandidate;

    QVector<int> candidateBauds;
    candidateBauds.reserve(401);
    for (int delta = 0; delta <= 200; ++delta) {
        int lower = nominalBaud - delta;
        int upper = nominalBaud + delta;
        if (delta == 0) {
            if (lower >= 300 && lower <= 1500) {
                candidateBauds.append(lower);
            }
            continue;
        }
        if (lower >= 300) {
            candidateBauds.append(lower);
        }
        if (upper <= 1500) {
            candidateBauds.append(upper);
        }
    }

    QVector<double> candidateStarts;
    candidateStarts.reserve(segments.size());
    for (const FskSignalSegment &segment : segments) {
        if (segment.signal == 0) {
            candidateStarts.append(segment.startSeconds);
        }
    }

    for (int baud : candidateBauds) {
        double bitLengthSeconds = 1.0 / static_cast<double>(baud);

        for (double startSeconds : candidateStarts) {
            QByteArray bytes = decodeBytesFromFskSegments(segments, startSeconds, bitLengthSeconds);
            if (bytes.size() < 2) {
                continue;
            }

            int syncIndex = -1;
            for (int i = 0; i + 1 < bytes.size(); ++i) {
                if ((quint8)bytes.at(i) == 0x55 && (quint8)bytes.at(i + 1) == 0x55) {
                    syncIndex = i;
                    break;
                }
            }
            if (syncIndex < 0) {
                continue;
            }

            if (syncIndex > 0) {
                bytes.remove(0, syncIndex);
            }

            if (bytes.size() > bestCandidate.bytes.size()) {
                bestCandidate.bytes = bytes;
                bestCandidate.baud = baud;
            }
        }
    }

    if (bestCandidate.bytes.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK decoder could not recover a standard byte stream near %1 baud.")
                    .arg(nominalBaud);
        }
        return false;
    }

    if (decodedBytes) {
        *decodedBytes = bestCandidate.bytes;
    }
    return true;
}

static bool decodeFskSignalsToBytes(const QVector<FskSignalRun> &sourceSignals, int baudRate,
                                    QByteArray *decodedBytes, int *detectedBaudRate, QString *errorMessage)
{
    if (decodedBytes) {
        decodedBytes->clear();
    }
    if (detectedBaudRate) {
        *detectedBaudRate = 0;
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    if (baudRate <= 0) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK decoder received an invalid baud rate (%1).").arg(baudRate);
        }
        return false;
    }

    QString deserializeError;
    int recoveredBaudRate = 0;
    if (decodeFskSignalsWithDeserializer(sourceSignals, decodedBytes, &recoveredBaudRate, &deserializeError)) {
        if (detectedBaudRate) {
            *detectedBaudRate = recoveredBaudRate;
        }
        return true;
    }

    return decodeFskSignalsBySampling(sourceSignals, baudRate, decodedBytes, errorMessage);
}

static bool decodeFskChunkToRecords(const QByteArray &data, int initialGapMs, int baudRate,
                                    QList<CassetteRecord> *records, int *totalDuration, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if ((data.size() & 1) != 0) {
        if (errorMessage) {
            *errorMessage = QObject::tr("FSK chunk length %1 is invalid.").arg(data.size());
        }
        return false;
    }

    QVector<FskSignalRun> segmentSignals;
    int gapDurationMs = initialGapMs;
    int signalLevel = 0;

    auto flushSegment = [&](void) -> bool {
        if (segmentSignals.isEmpty()) {
            return true;
        }

        QByteArray bytes;
        QString decodeError;
        int detectedBaudRate = baudRate;
        if (!decodeFskSignalsToBytes(segmentSignals, baudRate, &bytes, &detectedBaudRate, &decodeError)) {
            if (errorMessage) {
                *errorMessage = decodeError;
            }
            return false;
        }

        CassetteRecord record;
        record.baudRate = detectedBaudRate > 0 ? detectedBaudRate : baudRate;
        record.data = bytes;
        record.gapDuration = gapDurationMs;
        record.totalDuration = gapDurationMs + (bytes.size() * 10000 + (record.baudRate / 2)) / record.baudRate;
        records->append(record);
        if (totalDuration) {
            *totalDuration += record.totalDuration;
        }

        segmentSignals.clear();
        return true;
    };

    for (int i = 0; i < data.size(); i += 2) {
        quint16 durationUnits = (quint8)data.at(i) | ((quint8)data.at(i + 1) << 8);
        double signalLengthSeconds = durationUnits / 10000.0;

        // Long MARK signals are inter-record gaps in standard "normal" tape
        // recordings, so we split one FSK chunk into standard cassette records.
        if (signalLevel == 1 && signalLengthSeconds >= 0.05) {
            if (!flushSegment()) {
                return false;
            }
            gapDurationMs = qMax(1, qRound(signalLengthSeconds * 1000.0));
        } else {
            segmentSignals.append({signalLevel, signalLengthSeconds});
        }

        signalLevel = !signalLevel;
    }

    return flushSegment();
}

/* SioDevice */
SioDevice::SioDevice(SioWorker *worker)
        :QObject()
{
    sio = worker;
    m_deviceNo = -1;
}

SioDevice::~SioDevice()
{
    if (m_deviceNo != -1) {
        sio->uninstallDevice(m_deviceNo);
    }
}

QString SioDevice::deviceName()
{
    return sio->deviceName(m_deviceNo);
}

/* SioWorker */

SioWorker::SioWorker()
        : QThread()
{
    deviceMutex = new QRecursiveMutex();
    for (int i=0; i <= 255; i++) {
        devices[i] = 0;
    }
    mPort = 0;
}

SioWorker::~SioWorker()
{
    for (int i=0; i <= 255; i++) {
        if (devices[i]) {
            delete devices[i];
        }
    }
    delete deviceMutex;
}

bool SioWorker::wait(unsigned long time)
{
    mustTerminate = true;

    if (mPort) {
        mPort->cancel();
    }

    bool result = QThread::wait(time);

    if (mPort) {
        delete mPort;
        mPort = 0;
    }

    return result;
}

void SioWorker::start(Priority p)
{
    switch (aspeqtSettings->backend()) {
        case 0:
            mPort = new StandardSerialPortBackend(0);
            break;
//        #ifndef Q_OS_ANDROID
//        case 1:
//            mPort = new AtariSioBackend(0);
//            break;
//        #endif
    }

    mustTerminate = false;
    QThread::start(p);
}

void SioWorker::run()
{
    connect(mPort, SIGNAL(statusChanged(QString)), this, SIGNAL(statusChanged(QString)));

    /* Open serial port */
    if (!mPort->open()) {
        return;
    }

    /* Process SIO commands until we're explicitly stopped */
    while (!mustTerminate) {

//        qDebug() << "!d" << tr("DBG -- SIOWORKER...");

        QByteArray cmd = mPort->readCommandFrame();
        if (mustTerminate) {
            break;
        }
        if (cmd.isEmpty()) {
            qCritical() << "!e" << tr("Cannot read command frame.");
            break;
        }
        /* Decode the command */
        quint8 no = (quint8)cmd[0];
        quint8 command = (quint8)cmd[1];
        quint16 aux = ((quint8)cmd[2]) + ((quint8)cmd[3] * 256);

        /* Redirect the command to the appropriate device */
        deviceMutex->lock();
        if (devices[no]) {
            if (devices[no]->tryLock()) {
                devices[no]->handleCommand(command, aux);
                devices[no]->unlock();
            } else {
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 ignored because the image explorer is open.")
                               .arg(deviceName(no))
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
            }
        } else {
            qDebug() << "!u" << tr("[%1] command: $%2, aux: $%3 ignored.")
                           .arg(deviceName(no))
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
        }
        deviceMutex->unlock();
        cmd.clear();
    }
    mPort->close();
}

void SioWorker::installDevice(quint8 no, SioDevice *device)
{
    deviceMutex->lock();
    if (devices[no]) {
        delete devices[no];
    }
    devices[no] = device;
    device->setDeviceNo(no);
    deviceMutex->unlock();
}

void SioWorker::uninstallDevice(quint8 no)
{
    deviceMutex->lock();
    if (devices[no]) {
        devices[no]->setDeviceNo(-1);
    }
    devices[no] = 0;
    deviceMutex->unlock();
}

void SioWorker::swapDevices(quint8 d1, quint8 d2)
{
    SioDevice *t1, *t2;

    deviceMutex->lock();
    t1 = devices[d1];
    t2 = devices[d2];
    uninstallDevice(d1);
    uninstallDevice(d2);
    if (t2) {
        installDevice(d1, t2);
    }
    if (t1) {
        installDevice(d2, t1);
    }
    deviceMutex->unlock();
}

SioDevice* SioWorker::getDevice(quint8 no)
{
    SioDevice *result;
    deviceMutex->lock();
    result = devices[no];
    deviceMutex->unlock();
    return result;
}

QString SioWorker::deviceName(int device)
{
    QString result;
    switch (device) {
        case -1:
            // It must be because of the piggy-backed autoboot
            result = tr("Disk 1 (below autoboot)");
            break;
        case DISK_BASE_CDEVIC+0x0:
        case DISK_BASE_CDEVIC+0x1:
        case DISK_BASE_CDEVIC+0x2:
        case DISK_BASE_CDEVIC+0x3:
        case DISK_BASE_CDEVIC+0x4:
        case DISK_BASE_CDEVIC+0x5:
        case DISK_BASE_CDEVIC+0x6:
        case DISK_BASE_CDEVIC+0x7:
        case DISK_BASE_CDEVIC+0x8:
        case DISK_BASE_CDEVIC+0x9:
        case DISK_BASE_CDEVIC+0xA:
        case DISK_BASE_CDEVIC+0xB:
        case DISK_BASE_CDEVIC+0xC:
        case DISK_BASE_CDEVIC+0xD:
        case DISK_BASE_CDEVIC+0xE:
            result = tr("Disk %1").arg(device & 0x0F);
            break;
        case PRINTER_BASE_CDEVIC+0:
        case PRINTER_BASE_CDEVIC+1:
        case PRINTER_BASE_CDEVIC+2:
        case PRINTER_BASE_CDEVIC+3:
            result = tr("Printer %1").arg((device & 0x0F) + 1);
            break;
        case SMART_CDEVIC:
            result = tr("Smart device (APE time + URL)");
            break;
        case ASPEQT_CLIENT_CDEVIC:
            result = tr("AspeQt Client");
            break;
        case RS232_BASE_CDEVIC+0:
        case RS232_BASE_CDEVIC+1:
        case RS232_BASE_CDEVIC+2:
        case RS232_BASE_CDEVIC+3:
            result = tr("RS232 %1").arg((device & 0x0F) +1);
            break;
        case PCLINK_CDEVIC:
            result = tr("PCLINK");
            break;
        default:
            result = tr("Device $%1").arg(device, 2, 16, QChar('0'));
            break;
    }
    return result;
}

/* CassetteWorker */

CassetteWorker::CassetteWorker()
    : QThread()
{
    mPort = 0;
    mustTerminate.lock();
}

CassetteWorker::~CassetteWorker()
{
}

bool CassetteWorker::inspectCasImage(const QString &fileName, bool *containsFsk, QString *errorMessage)
{
    if (containsFsk) {
        *containsFsk = false;
    }
    if (errorMessage) {
        errorMessage->clear();
    }

    QFile casFile(fileName);

    if (!casFile.open(QFile::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = tr("Cannot open '%1': %2").arg(fileName).arg(casFile.errorString());
        }
        return false;
    }

    QByteArray header = casFile.read(8);
    if (header.length() != 8) {
        if (errorMessage) {
            *errorMessage = tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        }
        return false;
    }

    quint32 magic = casChunkMagic(header);
    quint16 length = casChunkLength(header);
    QByteArray data = casFile.read(length);
    if (data.length() != length) {
        if (errorMessage) {
            *errorMessage = tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        }
        return false;
    }

    if (magic != 0x494a5546) { // "FUJI"
        if (errorMessage) {
            *errorMessage = tr("Cannot open '%1': The header does not match.").arg(fileName);
        }
        return false;
    }

    while (!casFile.atEnd()) {
        header = casFile.read(8);
        if (header.isEmpty()) {
            break;
        }
        if (header.length() != 8) {
            if (errorMessage) {
                *errorMessage = tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            }
            return false;
        }

        magic = casChunkMagic(header);
        length = casChunkLength(header);
        data = casFile.read(length);
        if (data.length() != length) {
            if (errorMessage) {
                *errorMessage = tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            }
            return false;
        }

        if (magic == 0x206b7366) { // "fsk "
            if (containsFsk) {
                *containsFsk = true;
            }
        } else if (magic != 0x64756162 && magic != 0x61746164) { // "baud", "data"
            if (errorMessage) {
                *errorMessage = tr("Cannot open '%1': Unknown chunk header %2 (%3).")
                        .arg(fileName)
                        .arg(magic)
                        .arg(casChunkName(magic));
            }
            return false;
        }
    }

    return true;
}

bool CassetteWorker::loadCasImage(const QString &fileName, CassettePlaybackEngine engine)
{
    mRecords.clear();

    QFile casFile(fileName);

    if (!casFile.open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    QByteArray header, data;
    uint magic;
    int length, aux;

    header = casFile.read(8);

    if (header.length() != 8) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    magic = casChunkMagic(header);
    length = casChunkLength(header);
    aux = casChunkAux(header);


    data = casFile.read(length);
    if (data.length() != length) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    /* Verify the header */
    if (magic != 0x494a5546) { // "FUJI"
        qCritical() << "!e" << tr("Cannot open '%1': The header does not match.").arg(fileName);
        return false;
    }

    if (!data.isEmpty()) {
        qDebug() << "!n" << tr("[Cassette]: File description '%2'.").arg(QString::fromLatin1(data));
    }

    int lastBaud = 600;
    mTotalDuration = 0;
    bool containsFsk = false;
    bool sawDataRecord = false;

    /* Read the cas file */
    do {
        header = casFile.read(8);

        if (header.length() != 8) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        magic = casChunkMagic(header);
        length = casChunkLength(header);
        aux = casChunkAux(header);

        data = casFile.read(length);
        if (data.length() != length) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        /* Verify the header */
        if (magic == 0x64756162) {          // "baud"
            if (aspeqtSettings->useCustomCasBaud()) {
                lastBaud = aspeqtSettings->customCasBaud();
            } else {
                lastBaud = aux;
            }
        } else if (magic == 0x61746164) {   // "data"
            CassetteRecord record;
            record.baudRate = lastBaud;
            record.data = data;
            record.gapDuration = aux;
            record.totalDuration = aux + (length * 10000 + (lastBaud/2))/lastBaud;
            mTotalDuration += record.totalDuration;
            mRecords.append(record);
            sawDataRecord = true;
        } else if (magic == 0x206b7366) {   // "fsk "
            containsFsk = true;
            if (engine == CassettePlaybackStandard) {
                qCritical() << "!e" << tr("Cannot open '%1': Unknown chunk header %2.")
                                      .arg(fileName)
                                      .arg(magic);
                return false;
            }
            QString decodeError;
            int oldCount = mRecords.count();
            if (!decodeFskChunkToRecords(data, aux, lastBaud, &mRecords, &mTotalDuration, &decodeError)) {
                qWarning() << "!w" << tr("[Cassette] FSK decode failed (%1). Falling back to compatibility skip for this chunk.")
                                  .arg(decodeError);
                continue;
            }
            if (mRecords.count() == oldCount) {
                qWarning() << "!w" << tr("[Cassette] FSK chunk did not produce playable standard records. Falling back to compatibility skip.");
                continue;
            }
            sawDataRecord = true;
            qDebug() << "!n" << tr("[Cassette] Decoded FSK chunk into %1 standard record(s) at %2 baud.")
                            .arg(mRecords.count() - oldCount)
                            .arg(lastBaud);
        } else {
            qCritical() << "!e" << tr("Cannot open '%1': Unknown chunk header %2.").arg(fileName).arg(magic);
            return false;
        }

    } while (!casFile.atEnd());

    if (engine == CassettePlaybackFskCompatibility && containsFsk && !sawDataRecord) {
        qCritical() << "!e" << tr("Cannot open '%1': FSK chunks were found, but there are no standard data records for compatibility playback.")
                              .arg(fileName);
        return false;
    }

    return true;
}

bool CassetteWorker::wait (unsigned long time)
{
    if (mPort) {
        mPort->cancel();
    }

    mustTerminate.unlock();

    bool result = QThread::wait(time);

    if (mPort) {
        mPort->close();
        delete mPort;
        mPort = 0;
    }

    return result;
}

void CassetteWorker::run()
{
    /* Open serial port */
    if (!mPort->open()) {
        return;
    }

    int lastBaud = 0;
    int block = 1;
    int remainingTime = mTotalDuration;

    QTime tm = QTime::currentTime();

    foreach (CassetteRecord record, mRecords) {
        if (lastBaud != record.baudRate) {
            lastBaud = record.baudRate;
            if (!mPort->setSpeed(lastBaud)) {
                return;
            }
        }
        emit statusChanged(remainingTime);
        qDebug() << "!n" << tr("[Cassette] Playing record %1 of %2 (%3 ms of gap + %4 bytes of data)")
                .arg(block)
                .arg(mRecords.count())
                .arg(record.gapDuration)
                .arg(record.data.length());
        tm = tm.addMSecs(record.gapDuration);
        int w = QTime::currentTime().msecsTo(tm);
        if (w < 0) {
            w = 0;
        }
        if (mustTerminate.tryLock(w)) {
            return;
        }
        tm = QTime::currentTime();
        tm = tm.addMSecs((record.data.length() * 10000 + (lastBaud/2))/lastBaud);
        for (int i=0; i < record.data.length(); i+=10) {
            mPort->writeRawFrame(record.data.mid(i, 10));
            if (mustTerminate.tryLock()) {
                return;
            }
        }
        block++;
        remainingTime -= record.totalDuration;
    }
    // Wait until last written bytes are transferred and then some (FTDI bug)
    int w = QTime::currentTime().msecsTo(tm);
    if (w < 0) {
        w = 0;
    }
    msleep(w + 500);
    mPort->close();
}

void CassetteWorker::start(Priority p)
{
    switch (aspeqtSettings->backend()) {
        case 0:
            mPort = new StandardSerialPortBackend(0);
            break;
//        #ifndef Q_OS_ANDROID
//        case 1:
//            mPort = new AtariSioBackend(0);
//            break;
//        #endif
    }
    QThread::start(p);
}
