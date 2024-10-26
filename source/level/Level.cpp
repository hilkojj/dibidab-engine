#include "Level.h"
#include "room/Room.h"

#include "../ecs/components/Player.dibidab.h"
#include "../ecs/templates/Template.h"
#include "../dibidab/dibidab.h"

#include <files/file_utils.h>
#include <gu/profiler.h>

#include <zlib.h>

std::function<dibidab::level::Room *(const json &)> dibidab::level::Level::customRoomLoader;

void dibidab::level::Level::setPaused(bool bInPaused)
{
    bPaused = bInPaused;
}

void dibidab::level::Level::initialize()
{
    int i = 0;
    for (auto &room : rooms)
    {
        room->roomI = i++;
        room->initialize(this);
    }
    initialized = true;
    if (rooms.size() > 0)
    {
        Room *spawnRoom = rooms[0];
        spawnRoom->getTemplate("Player").create(false);
    }
}

void dibidab::level::Level::update(double deltaTime)
{
    gu::profiler::Zone levelUpdateZone("level update");
    updating = true;

    time += deltaTime;
    for (int i = 0; i < rooms.size(); i++)
    {
        Room *room = rooms[i];
        if (room->entities.empty<ecs::Player>())
        {
            continue;
        }
        room->update(deltaTime);
    }

    updating = false;
}

#define DEFAULT_LEVEL_PATH "assets/default_level.lvl"

dibidab::level::Level::~Level()
{
    if (bSaveOnDestruct)
        save(loadedFromFile.empty() ? DEFAULT_LEVEL_PATH : loadedFromFile.c_str());

    for (auto r : rooms)
        r->events.emit(0, "BeforeDelete");
    for (auto r : rooms)
        delete r;
}

dibidab::level::Room &dibidab::level::Level::getRoom(int i)
{
    if (i < 0 || i >= rooms.size()) throw gu_err("index out of bounds");
    return *rooms[i];
}

const dibidab::level::Room &dibidab::level::Level::getRoom(int i) const
{
    if (i < 0 || i >= rooms.size()) throw gu_err("index out of bounds");
    return *rooms[i];
}

void dibidab::level::to_json(json &j, const Level &lvl)
{
    j = json::object({ { "rooms", json::array() } });
    for (Room *room : lvl.rooms)
    {
        if (room->isPersistent())
        {
            json &roomJ = j["rooms"].emplace_back();
            room->exportJsonData(roomJ);
        }
    }
}

void dibidab::level::from_json(const json &j, Level &lvl)
{
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

void dibidab::level::Level::deleteRoom(int i)
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

void dibidab::level::Level::save(const char *path) const
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

dibidab::level::Level::Level(const char *filePath) : loadedFromFile(filePath)
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

dibidab::level::Room *dibidab::level::Level::getRoomByName(const char *n)
{
    for (auto &r : rooms)
        if (r->name == n)
            return r;
    return nullptr;
}

const dibidab::level::Room *dibidab::level::Level::getRoomByName(const char *n) const
{
    return ((Level *) this)->getRoomByName(n);
}

void dibidab::level::Level::addRoom(Room *r)
{
    assert(r->level == nullptr);

    rooms.push_back(r);

    if (initialized)
    {
        r->roomI = rooms.size() - 1;
        r->initialize(this);
    }
}
