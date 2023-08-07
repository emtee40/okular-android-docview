/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "audioplayer.h"

// qt/kde includes
#include <KLocalizedString>
#include <kio/storedtransferjob.h>
#include <QAudio>
#include <QAudioOutput>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QRandomGenerator>

#include "config-okular.h"


// local includes
#include "action.h"
#include "debug_p.h"
#include "document.h"
#include "sound.h"
#include <stdlib.h>

using namespace Okular;

class SoundInfo;
class PlayData;

class PlayData;
class SoundInfo;

namespace Okular
{
class AudioPlayerPrivate
{
public:
    explicit AudioPlayerPrivate(AudioPlayer *qq);

    ~AudioPlayerPrivate();

    int newId() const;
    bool play(const SoundInfo &si);
    void stopPlayings();
    void dataDownloaded(KJob *job, QByteArray &data);

    void finished(int);

    AudioPlayer *q;

    QHash<int, PlayData *> m_playing;
    QUrl m_currentDocument;
    AudioPlayer::State m_state;
    // Map of KJob to play data for lookup when downloading, etc.
    QMap<KJob *, PlayData *> m_playDataByJob;
};
}

// helper class used to store info about a sound to be played
class SoundInfo
{
public:
    explicit SoundInfo(const Sound *s = nullptr, const SoundAction *ls = nullptr)
        : sound(s)
        , volume(0.5)
        , synchronous(false)
        , repeat(false)
        , mix(false)
    {
        if (ls) {
            volume = ls->volume();
            synchronous = ls->synchronous();
            repeat = ls->repeat();
            mix = ls->mix();
        }
    }

    const Sound *sound;
    double volume;
    bool synchronous;
    bool repeat;
    bool mix;
};

class PlayData
{
public:
    PlayData()
        : m_output(nullptr)
        , m_device(nullptr)
    {
    }

    void play()
    {
        if (m_device) {
            m_device->open(QIODevice::ReadOnly);
            m_output->start(m_device);
        }
    }

    ~PlayData()
    {
        m_output->stop();
        delete m_output;
        delete m_device;
    }

    PlayData(const PlayData &) = delete;
    PlayData &operator=(const PlayData &) = delete;

    QAudioOutput *m_output;
    QIODevice *m_device;
    SoundInfo m_info;
};

AudioPlayerPrivate::AudioPlayerPrivate(AudioPlayer *qq)
    : q(qq)
    , m_state(AudioPlayer::StoppedState)
{
}

AudioPlayerPrivate::~AudioPlayerPrivate()
{
    stopPlayings();
}

int AudioPlayerPrivate::newId() const
{
    auto random = QRandomGenerator::global();
    int newid = 0;
    QHash<int, PlayData *>::const_iterator it;
    QHash<int, PlayData *>::const_iterator itEnd = m_playing.constEnd();
    do {
        newid = random->bounded(RAND_MAX);
        it = m_playing.constFind(newid);
    } while (it != itEnd);
    return newid;
}

bool AudioPlayerPrivate::play(const SoundInfo &si)
{
    qCDebug(OkularCoreDebug) << "play called";
    PlayData *data = new PlayData();
    data->m_info = si;
    QAudioFormat format;
    format.setSampleRate(data->m_info.sound->samplingRate());
    format.setChannelCount(data->m_info.sound->channels());
    format.setSampleSize(data->m_info.sound->bitsPerSample());
//    format.setByteOrder(QAudioFormat::LittleEndian);
//    format.setSampleType(QAudioFormat::UnSignedInt);
    data->m_output = new QAudioOutput(format, nullptr);
    data->m_output->setVolume(data->m_info.volume);
    bool valid = false;
    bool downloading = false;
    qCDebug(OkularCoreDebug) << "Audio type: " << data->m_info.sound->soundType();

    switch (data->m_info.sound->soundType()) {
    case Sound::External: {
        QString url = si.sound->url();
        qCDebug(OkularCoreDebug) << "External," << url;
        if (!url.isEmpty()) {
            const QUrl newurl = QUrl::fromUserInput(url, m_currentDocument.adjusted(QUrl::RemoveFilename).toLocalFile());
            KIO::Job *job = KIO::storedGet(newurl);

            QObject::connect(job, &KJob::result, q, &AudioPlayer::downloadFinished);

            job->exec();
            m_playDataByJob.insert(job, data);

            valid = true;
            downloading = true;
        }
        break;
    }
    case Sound::Embedded: {
        QByteArray filedata = si.sound->data();
        qCDebug(OkularCoreDebug) << "Embedded," << filedata.length();
        if (!filedata.isEmpty()) {
            qCDebug(OkularCoreDebug) << "Mediaoutput:" << data->m_output;
            int newid = newId();
            QObject::connect(data->m_output, &QAudioOutput::stateChanged, q, [this, newid]() { finished(newid); });
            QBuffer *buffer = new QBuffer();
            buffer->setData(filedata);
            data->m_device = buffer;
            m_playing.insert(newid, data);
            valid = true;
        }
        break;
    }
    }
    if (!valid) {
        delete data;
        data = nullptr;
    }
    if (data && !downloading) {
        qCDebug(OkularCoreDebug) << "PLAY";
        data->play();
        m_state = AudioPlayer::PlayingState;
    }
    return valid;
}

void AudioPlayerPrivate::stopPlayings()
{
    qDeleteAll(m_playing);
    m_playing.clear();
    m_state = AudioPlayer::StoppedState;
}

void AudioPlayerPrivate::dataDownloaded(KJob *job, QByteArray &data)
{
    if (!m_playDataByJob.contains(job)) {
        qCWarning(OkularCoreDebug) << "Unable to match downloaded data to PlayData, not playing";
        // Error, do nothing
        return;
    }

    PlayData *playData = m_playDataByJob.value(job);
    QBuffer *buffer = new QBuffer(&data, nullptr);
    buffer->setData(data);

    playData->m_device = buffer;
    playData->play();
    int newid = newId();
    QObject::connect(playData->m_output, &QAudioOutput::stateChanged, q, [this, newid]() { finished(newid); });
    m_playing.insert(newid, playData);
    m_state = AudioPlayer::PlayingState;
}

void AudioPlayerPrivate::finished(int id)
{
    QHash<int, PlayData *>::iterator it = m_playing.find(id);
    if (it == m_playing.end()) {
        return;
    }

    SoundInfo si = it.value()->m_info;
    // if the sound must be repeated indefinitely, then start the playback
    // again, otherwise destroy the PlayData as it's no more useful
    if (si.repeat) {
        it.value()->play();
    } else {
        delete it.value();
        m_playing.erase(it);
        m_state = AudioPlayer::StoppedState;
    }
    qCDebug(OkularCoreDebug) << "finished," << m_playing.count();
}

AudioPlayer::AudioPlayer()
    : QObject()
    , d(new AudioPlayerPrivate(this))
{
}

AudioPlayer::~AudioPlayer()
{
    delete d;
}

AudioPlayer *AudioPlayer::instance()
{
    static AudioPlayer ap;
    return &ap;
}

void AudioPlayer::playSound(const Sound *sound, const SoundAction *linksound)
{
    // we can't play null pointers ;)
    if (!sound) {
        return;
    }

    // we don't play external sounds for remote documents
    if (sound->soundType() == Sound::External && !d->m_currentDocument.isLocalFile()) {
        qCDebug(OkularCoreDebug) << "Tried to play external sound for remote document, bailing out.";
        return;
    }

    qCDebug(OkularCoreDebug) << "Playing sound";
    SoundInfo si(sound, linksound);

    // if the mix flag of the new sound is false, then the currently playing
    // sounds must be stopped.
    if (!si.mix) {
        d->stopPlayings();
    }

    d->play(si);
}

void AudioPlayer::downloadFinished(KJob *job)
{
    if (job->error()) {
        qCDebug(OkularCoreDebug) << "Download finished, but got an error: " << job->errorString();
        return;
    }

    const KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob *>(job);

    if (storedJob) {
        QByteArray jobData = storedJob->data();
        d->dataDownloaded(job, jobData);
    }

}

void AudioPlayer::stopPlaybacks()
{
    d->stopPlayings();
}

AudioPlayer::State AudioPlayer::state() const
{
    return d->m_state;
}

void AudioPlayer::resetDocument()
{
    d->m_currentDocument = {};
}

void AudioPlayer::setDocument(const QUrl &url, Okular::Document *document)
{
    Q_UNUSED(document);
    d->m_currentDocument = url;
}

#include "moc_audioplayer.cpp"
