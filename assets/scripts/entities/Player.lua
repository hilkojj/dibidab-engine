description("This is the player description")

function create(player)
    setName(player, "Yo yo player")
    setComponents(player, {
    })
    print("hey!")

    local child = createChild(player, "Henk")

    local testEntity = createEntity()
    setName(testEntity, "Test entity")
end
