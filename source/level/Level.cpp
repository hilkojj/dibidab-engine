
#include <files/file_utils.h>
#include <gu/game_utils.h>
#include <gu/profiler.h>
#include <utils/gu_error.h>
#include <cstring>
#include <zlib.h>
#include <level/room/Room.h>


#include "Level.h"
#include "../generated/PlayerControlled.hpp"
#include "../game/dibidab.h"

std::function<Room *(const json &)> Level::customRoomLoader;

void Level::setPaused(bool bInPaused)
{
    bPaused = bInPaused;
}

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

    if (dibidab::settings.bLimitUpdatesPerSec)
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
        time += deltaTime;
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
        if (!room->isPersistent())
        {
            continue;
        }
        json &roomJ = j["rooms"].emplace_back();
        room->exportJsonData(roomJ);
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
            r->loadJsonData(rJ);
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

typedef uint64 level_data_length_type;

void Level::save(const char *path) const
{
    std::vector<unsigned char> data(sizeof(level_data_length_type), 0u);

    json::to_cbor(*this, data);
    *((level_data_length_type *) &data[0]) = data.size() - sizeof(level_data_length_type);

    for (Room *room : rooms)
    {
        if (!room->isPersistent())
        {
            continue;
        }
        data.resize(data.size() + sizeof(level_data_length_type));
        const level_data_length_type dataSizeBeforeRoom = data.size();
        room->exportBinaryData(data);

        const level_data_length_type roomDataSize = data.size() - dataSizeBeforeRoom;
        *((level_data_length_type *) (&data[dataSizeBeforeRoom - sizeof(level_data_length_type)])) = roomDataSize;
    }

    std::vector<uint8> compressedData(long(double(12 + data.size()) * 1.1));
    unsigned long compressedDataSize = compressedData.size();

    int zResult = compress(&compressedData[0], &compressedDataSize, (const uint8 *) &data[0], data.size());

    if (zResult != Z_OK)
        throw gu_err("Error while compressing level");

    // store original data size at end of buffer:
    compressedData.resize(compressedDataSize + sizeof(int));
    int *originalDataSize = (int *) &compressedData[compressedDataSize];
    *originalDataSize = int(data.size());

    fu::writeBinary(path ? path : loadedFromFile.c_str(), (char *) compressedData.data(), compressedData.size());
}

Level::Level(const char *filePath) : loadedFromFile(filePath)
{
    if (!fu::exists(filePath))
    {
        std::cout << "No level file found at " << filePath << ", creating empty Level...\n";
        return;
    }
    try // Todo: create a loadCompressed() and saveCompressed() function in gu
    {
        auto compressedData = fu::readBinary(filePath);

        unsigned long compressedDataSize = compressedData.size() - sizeof(int);

        unsigned long originalDataSize = *((int *) &compressedData[compressedDataSize]);
        unsigned long originalDataSize_ = originalDataSize;

        std::vector<uint8> uncompressedData(originalDataSize);

        int zResult = uncompress(&uncompressedData[0], &originalDataSize, &compressedData[0], compressedDataSize);

        if (originalDataSize != originalDataSize_)
        {
            throw gu_err("Length of uncompressed data does not match length of original data");
        }

        if (zResult != Z_OK)
        {
            throw gu_err("Error while UNcompressing");
        }

        if (uncompressedData.size() <= sizeof(level_data_length_type))
        {
            throw gu_err("Level file does barely contain any data!");
        }
        const level_data_length_type jsonDataLength = *((level_data_length_type *) &uncompressedData[0]);

        const level_data_length_type binaryBegin = sizeof(level_data_length_type) + jsonDataLength;

        if (binaryBegin > uncompressedData.size())
        {
            throw gu_err("Level file does not contain as much json data as described!");
        }

        json j = json::from_cbor(
            &uncompressedData[sizeof(level_data_length_type)],
            &uncompressedData[binaryBegin]
        );
        from_json(j, *this);

        level_data_length_type nextRoomBinaryBegin = binaryBegin;
        for (Room *room : rooms)
        {
            if (nextRoomBinaryBegin + sizeof(level_data_length_type) >= uncompressedData.size())
            {
                break;
            }
            const level_data_length_type roomBinaryBegin = nextRoomBinaryBegin;
            const level_data_length_type roomBinaryDataLength = *((level_data_length_type *) &uncompressedData[roomBinaryBegin]);
            nextRoomBinaryBegin += roomBinaryDataLength + sizeof(level_data_length_type);
            if (nextRoomBinaryBegin > uncompressedData.size())
            {
                throw gu_err("Level file does not contain as much binary room data as described for room #" + std::to_string(room->roomI));
            }
            room->loadBinaryData(&uncompressedData[roomBinaryBegin + sizeof(level_data_length_type)], roomBinaryDataLength);
        }
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
    return nullptr;
}

const Room *Level::getRoomByName(const char *n) const
{
    return ((Level *) this)->getRoomByName(n);
}

void Level::addRoom(Room *r)
{
    assert(r->level == nullptr);

    rooms.push_back(r);

    if (initialized)
    {
        r->roomI = rooms.size() - 1;
        r->initialize(this);
    }
}
