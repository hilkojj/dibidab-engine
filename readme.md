# Dibidab Engine

Dibidab is a small game engine with some nice features that I'd like to reuse.

### Features:
- **ECS based** (Entities are built out of Components, using [EnTT](https://github.com/skypjack/entt))
- [**All components are serializable**](#magic-serializable-components)
- **Saving and loading Levels** (entities and their components)
- **SaveGames** (saving custom progress data, or custom data specific to an entity)
- **Multiplayer** (automagically sending entities and component updates over the network)
- [**Lua scripting**](#lua-scripting) (create and update entities and their components using the Lua API)
- **Live reloading** (reload assets **and scripts** while your game is running)
- [**Entity Inspector**](#entity-inspector) (inspect and change component values in a GUI)
- [**Profiler**](#profiler) (get an overview of what is taking up the most time in your game-loop)
- **Cross Platform** (Linux, Windows, **and Web browsers**)

### Missing Features:
- Documentation
- Ease-of-use
- Unit-testing
- all the boring stuff
- seriously, don't use this game engine, it's a personal project


## Magic serializable Components
Usually you would define a component as a C-style struct like this:
```C++
struct MyComponent {
    int a;
    float x = -30;
    std::list<std::string> myHobbies = {"writing shit code", "sleeping"};
};
```
But using C-structs there's no easy way to...
- send `MyComponent` over the network without breaking pointers
- serialize `MyComponent` to json, used for saving and loading
- read and change `MyComponent`'s values from within a Lua script

 So instead we define components in YAML files:
```yaml
MyComponent:
  - a: int
  - x: [float, -30]
  - myHobbies: std::list<std::string>
```
*Before* compilation these Yaml files automagically get converted into C++ structs ([thanks to Niek](https://github.com/dibidabidab/lua-serde)).
Alongside these structs, functions are generated to make the serializing and lua magic stuff listed above possible.


## Lua Scripting
I hate it when I have to recompile my game every time I change a variable or a little bit of game logic.
It slows down development a lot*.

So with this game engine you can create and update your entity's components inside a (live-reloadable) Lua script!

Example of an entity-template written in Lua:
```lua
-- On game restart: recreate this entity with the same template, at the original spawn position
persistenceMode(TEMPLATE | SPAWN_POS | REVIVE) 

function create(enemy)
    setComponents(enemy, {
        Position {
            x = 3, y = 50
        },
        Physics(),
        Health {
            maxHealth = 4
        },
        AsepriteView {
            sprite = "sprites/enemy"
        }
    })

    onEntityEvent(enemy, "Attacked", function(attack)

        local health = component.Health.getFor(enemy)

        print("OUCH!", attack.points, health.currHealth, "/", health.maxHealth)
    end)
    
    setUpdateFunction(enemy, .1, function(deltaTime)
        -- do stuff every 0.1 second
    end)
end
```

<sup><sup>* You know what slows down development even more? Writing your own Game Engine in C++ with a scripting API that should speed up development.</sup></sup>


## Entity Inspector
Inspect your components as if it were a JSON tree:

![](https://imgur.com/91eVQg9l.png)

## Profiler

```c++
void updateLevel()
{
    gu::profiler::Zone zone("level update");
    
    doStuff();
}

void loop()
{
    {
        gu::profiler::Zone zone("logic");
    
        updateLevel();
    }
    {
        gu::profiler::Zone zone("render");
    
        // render stuff...
    }
}
```

![](https://imgur.com/NZcTBPy.png)
