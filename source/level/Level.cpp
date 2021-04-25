
#include <files/file.h>
#include <gu/game_utils.h>
#include <utils/gu_error.h>
#include <cstring>
#include <zlib.h>

#include "Level.h"
#include "../generated/PlayerControlled.hpp"
#include "../game/dibidab.h"

std::function<Room *(const json &)> Level::customRoomLoader;

void Level::initialize()
{
    int i = 0;
    for (auto &room : rooms)
    {
        room->roomI = i++;
        room->initialize(this);
    }
    initialized = true;
}

void Level::update(double deltaTime)
{
    gu::profiler::Zone levelUpdateZone("level update");
    updating = true;

    auto update = [&] (double roomDeltaTime) {
        /*
         * This piece of code will update rooms that have a player in them.
         * Rooms are checked twice for a player in them, because a Player might have been spawned during another Room's update
         */

        std::vector<bool> skippedRoom(rooms.size(), true);

        for (int repeat = 0; repeat < 2; repeat++)
        {
            for (int i = 0; i < rooms.size(); i++)
            {
                auto room = rooms[i];
                if (!skippedRoom[i] || room->entities.empty<PlayerControlled>())
                {
                    skippedRoom[i] = true;
                    continue;
                }
                skippedRoom[i] = false;
                room->update(roomDeltaTime);
            }
        }
    };

    if (dibidab::settings.limitUpdatesPerSec)
    {

        updateAccumulator += deltaTime;

        static float MIN_DELTA_TIME = 1. / MAX_UPDATES_PER_SEC;
        static float MAX_DELTA_TIME = 1. / MIN_UPDATES_PER_SEC;

        int updates = 0;

        while (updateAccumulator >= MIN_DELTA_TIME && updates++ < MAX_UPDATES_PER_FRAME)
        {
            float roomDeltaTime = min(updateAccumulator, MAX_DELTA_TIME);
            time += roomDeltaTime;

            update(roomDeltaTime);

            updateAccumulator -= roomDeltaTime;
        }

        if (updateAccumulator > 2.)
        {
            std::cerr << "Level::update() can't reach MIN_UPDATES_PER_SEC! Skipping " << updateAccumulator << "sec!" << std::endl;
            updateAccumulator = 0;
        }
    }
    else
    {
        update(deltaTime);
    }

    updating = false;
}

#define DEFAULT_LEVEL_PATH "assets/default_level.lvl"

Level::~Level()
{
    if (saveOnDestruct)
        save(loadedFromFile.empty() ? DEFAULT_LEVEL_PATH : loadedFromFile.c_str());

    for (auto r : rooms)
        r->events.emit(0, "BeforeDelete");
    for (auto r : rooms)
        delete r;
}

Room &Level::getRoom(int i)
{
    if (i < 0 || i >= rooms.size()) throw gu_err("index out of bounds");
    return *rooms[i];
}

const Room &Level::getRoom(int i) const
{
    if (i < 0 || i >= rooms.size()) throw gu_err("index out of bounds");
    return *rooms[i];
}

void to_json(json &j, const Level &lvl)
{
    j = json::object({{"spawnRoom", lvl.spawnRoom}, {"rooms", json::array()}});
    for (auto room : lvl.rooms)
    {
        json &roomJ = j["rooms"].emplace_back();
        room->toJson(roomJ);
    }
}

void from_json(const json &j, Level &lvl)
{
    lvl.spawnRoom = j.at("spawnRoom");
    const json &roomsJson = j.at("rooms");

    for (const auto &rJ : roomsJson)
    {
        if (Level::customRoomLoader)
            lvl.addRoom(Level::customRoomLoader(rJ));
        else
        {
            Room *r = new Room;
            r->fromJson(rJ);
            lvl.addRoom(r);
        }
    }
}

void Level::deleteRoom(int i)
{
    assert(i < rooms.size());

    Room *old = rooms[i];
    beforeRoomDeletion(old);

    rooms[i] = rooms.back();
    rooms[i]->roomI = i;

    delete old;
    rooms.pop_back();
}

void Level::save(const char *path) const
{
//    std::vector<unsigned char> data;

    auto data = json(*this).dump();

//    json::to_cbor(*this, data);

    std::vector<uint8> compressedData(12 + data.size() * 1.1);
    unsigned long compressedDataSize = compressedData.size();

    int zResult = compress(&compressedData[0], &compressedDataSize, (const uint8 *) &data[0], data.size());

    if (zResult != Z_OK)
        throw gu_err("Error while compressing level");

    // store original data size at end of buffer:
    compressedData.resize(compressedDataSize + sizeof(int));
    int *originalDataSize = (int *) &compressedData[compressedDataSize];
    *originalDataSize = int(data.size());

    File::writeBinary(path, compressedData);
}

Level::Level(const char *filePath) : loadedFromFile(filePath)
{
    if (!File::exists(filePath))
    {
        std::cout << "No level file found at " << filePath << ", creating empty Level...\n";
        return;
    }
    try // Todo: create a loadCompressed() and saveCompressed() function in gu
    {
        auto compressedData = File::readBinary(filePath);

        unsigned long compressedDataSize = compressedData.size() - sizeof(int);

        unsigned long originalDataSize = *((int *) &compressedData[compressedDataSize]);
        unsigned long originalDataSize_ = originalDataSize;

        std::vector<uint8> uncompressedData(originalDataSize);

        int zResult = uncompress(&uncompressedData[0], &originalDataSize, &compressedData[0], compressedDataSize);

        if (originalDataSize != originalDataSize_)
            throw gu_err("Length of uncompressed data does not match length of original data");

        if (zResult != Z_OK)
            throw gu_err("Error while UNcompressing");

        json j = json::parse(uncompressedData.begin(), uncompressedData.end()); //json::from_cbor(uncompressedData.begin(), uncompressedData.end());
        from_json(j, *this);
    }
    catch (std::exception &e)
    {
        throw gu_err("Failed to load level from " + loadedFromFile + ":\n" + e.what());
    }
}

Room *Level::getRoomByName(const char *n)
{
    for (auto &r : rooms)
        if (r->name == n)
            return r;
    return NULL;
}

const Room *Level::getRoomByName(const char *n) const
{
    return ((Level *) this)->getRoomByName(n);
}

void Level::addRoom(Room *r)
{
    assert(r->level == NULL);

    rooms.push_back(r);

    if (initialized)
    {
        r->roomI = rooms.size() - 1;
        r->initialize(this);
    }
}
