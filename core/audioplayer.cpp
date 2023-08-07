/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "audioplayer.h"

// qt/kde includes
#include <KLocalizedString>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QMediaPlayer>
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

    void stateChanged(int, QMediaPlayer::State);

    AudioPlayer *q;

    QHash<int, PlayData *> m_playing;
    QUrl m_currentDocument;
    AudioPlayer::State m_state;
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
        : m_player(nullptr)
    {
    }

    void play()
    {
        m_player->play();
    }

    ~PlayData()
    {
        // Block signals so stateChanged wont get called from stop()
        m_player->blockSignals(true);
        m_player->stop();
        delete m_player;
    }

    PlayData(const PlayData &) = delete;
    PlayData &operator=(const PlayData &) = delete;

    QMediaPlayer *m_player;
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
    data->m_player = new QMediaPlayer();
    data->m_player->setVolume(data->m_info.volume);
    bool valid = false;

    switch (data->m_info.sound->soundType()) {
    case Sound::External: {
        QString url = si.sound->url();
        qCDebug(OkularCoreDebug) << "External," << url;
        if (!url.isEmpty()) {
            int newid = newId();
            const QUrl newurl = QUrl::fromUserInput(url, m_currentDocument.adjusted(QUrl::RemoveFilename).toLocalFile());
            data->m_player->setMedia(newurl);
            QObject::connect(data->m_player, &QMediaPlayer::stateChanged, q, [this, newid](QMediaPlayer::State state) { stateChanged(newid, state); });
            m_playing.insert(newid, data);
            valid = true;
        }
        break;
    }
    case Sound::Embedded: {
        QByteArray filedata = si.sound->data();
        qCDebug(OkularCoreDebug) << "Embedded," << filedata.length();
        if (!filedata.isEmpty()) {
            int newid = newId();
            QObject::connect(data->m_player, &QMediaPlayer::stateChanged, q, [this, newid](QMediaPlayer::State state) { stateChanged(newid, state); });
            QBuffer *buffer = new QBuffer(q);
            buffer->setData(filedata);
            buffer->open(QBuffer::ReadOnly);
            data->m_player->setMedia(QMediaContent(), buffer);
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
    if (data) {
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

void AudioPlayerPrivate::stateChanged(int id, QMediaPlayer::State state)
{
    QHash<int, PlayData *>::iterator it = m_playing.find(id);
    if (it == m_playing.end()) {
        return;
    }

    if (state == QMediaPlayer::StoppedState) {
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
