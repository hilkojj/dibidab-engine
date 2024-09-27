
#include "AudioSystem.h"
#include "../../generated/PlayerControlled.hpp"

void AudioSystem::init(EntityEngine *engine)
{
    if (Room *room = dynamic_cast<Room *>(engine))
    {
        onPlayerLeft = room->getLevel().onPlayerLeftRoom += [&, room] (Room *r, auto) {
            if (r != room)
                return;

            if (!r->entities.empty<LocalPlayer>())
                return;

            int paused = 0;
            r->entities.view<SoundSpeaker>().each([&](SoundSpeaker &speaker) {
                if (speaker.pauseOnLeavingRoom && speaker.source && !speaker.source->isPaused())
                {
                    speaker.source->pause();
                    paused++;
                }
            });
            if (paused > 0)
                std::cout << "Paused " << paused << " sounds in room " << r->getIndexInLevel() << std::endl;
        };
    }
}

void AudioSystem::update(double deltaTime, EntityEngine *room)
{
    int resumed = 0;
    bool roomActive = !room->entities.empty<LocalPlayer>();
    room->entities.view<SoundSpeaker>().each([&](auto e, SoundSpeaker &speaker) {

        if (roomActive && !speaker.paused && speaker.source && speaker.source->isPaused())
        {
            speaker.source->play();
            resumed++;
        }

        auto hash = speaker.getHash();
        if (hash != speaker.prevHash)
        {
            // speaker settings have changed, update SoundSource:

            if (!speaker.source && speaker.sound.isSet())
            {
                speaker.source = std::make_shared<au::SoundSource>(speaker.sound.get());
                speaker.source->play();
            }

            if (speaker.source)
            {
                if (roomActive && speaker.paused != speaker.source->isPaused())
                {
                    if (speaker.paused)
                        speaker.source->pause();
                    else
                        speaker.source->play();
                }

                speaker.source->setVolume(speaker.volume);
                speaker.source->setLooping(speaker.looping);
                speaker.source->setPitch(speaker.pitch);
            }
            speaker.prevHash = hash;
        }
        if (!speaker.looping && speaker.source && speaker.source->hasStopped())
            room->entities.remove<SoundSpeaker>(e);
    });
    if (resumed > 0)
        std::cout << "Resumed " << resumed << " sounds in room" << std::endl;
}

