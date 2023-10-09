
#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "room/Room.h"

/**
 * A level contains one or more Rooms.
 */
class Level
{
    double time = 0;
    bool bPaused = false;
    std::vector<Room *> rooms;

    bool updating = false, initialized = false;
    float updateAccumulator = 0;
    constexpr static int    // todo, make this overridable:
        MAX_UPDATES_PER_SEC = 60,
        MIN_UPDATES_PER_SEC = 60,
        MAX_UPDATES_PER_FRAME = 1;

    friend void to_json(json& j, const Level& lvl);
    friend void from_json(const json& j, Level& lvl);

  public:

    /**
     * By default a Level loads Rooms by doing `r = new Room(); r->fromJson(j);`
     *
     * If you want to use (a) custom room type(s) in your game however,
     * you can set `customRoomLoader` to a function that returns a new room of your type.
     *
     * You should use the given json object for calling `fromJson()` on your newly created room.
     */
    static std::function<Room *(const json &)> customRoomLoader;

    const std::string loadedFromFile;
    bool saveOnDestruct = true;

    std::string spawnRoom;

    Level() = default;

    Level(const char *filePath);

    delegate<void(Room *, int playerId)> onPlayerEnteredRoom, onPlayerLeftRoom;
    delegate<void(Room *)> beforeRoomDeletion;

    int getNrOfRooms() const { return rooms.size(); }

    Room &getRoom(int i);

    const Room &getRoom(int i) const;

    Room *getRoomByName(const char *);

    const Room *getRoomByName(const char *) const;

    void deleteRoom(int i);

    void addRoom(Room *);

    double getTime() const { return time; }

    bool isPaused() const { return bPaused; }

    void setPaused(bool bPaused);

    bool isUpdating() const { return updating; }

    void initialize();

    /**
     * Updates the level and it's Rooms.
     *
     * @param deltaTime Time passed since previous update
     */
    void update(double deltaTime);

    void save(const char *path) const;

    ~Level();

};

void to_json(json& j, const Level& lvl);

void from_json(const json& j, Level& lvl);

#endif
