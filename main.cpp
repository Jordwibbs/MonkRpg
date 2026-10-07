#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <limits>

// Reads a whole line and only accepts a whole number between min and max.
// Using getline everywhere (instead of mixing it with cin >>) means no leftover
// newlines, and typing letters can never put cin into a failed state.
int readChoice(int min, int max) {
    while (true) {
        std::string input;
        if (!std::getline(std::cin, input)) std::exit(0); // input stream closed
        try {
            int choice = std::stoi(input);
            if (choice >= min && choice <= max) return choice;
        } catch (...) {}
        std::cout << "Please enter a number from " << min << " to " << max << ": ";
    }
}

// Basic/important classes
class Game;
class Room;
class GameState;

// ----------- Coord Struct -----------
struct Coord {
    int x, y;
    bool operator==(const Coord& other) const { return x == other.x && y == other.y; }
};

// ----------- Room Base + Derived Classes -----------
class Room {
protected:
    std::string type;
    std::vector<Room*> connections;
    Coord coord;
    bool visited = false; // each room's event only happens once

public:
    Room(std::string t, Coord c) : type(t), coord(c) {}
    virtual ~Room() {}

    void connect(Room* other) {
        if (std::find(connections.begin(), connections.end(), other) == connections.end()) {
            connections.push_back(other);
            other->connections.push_back(this);
        }
    }

    [[nodiscard]] std::string getType() const { return type; }
    [[nodiscard]] const std::vector<Room*>& getConnections() const { return connections; }
    [[nodiscard]] Coord getCoord() const { return coord; }
    [[nodiscard]] bool isVisited() const { return visited; }
    void markVisited() { visited = true; }

    virtual void enter(Game* game) = 0;
};

class EmptyRoom : public Room {
public:
    explicit EmptyRoom(Coord c) : Room("Empty", c) {}
    void enter(Game* game) override;
};

class UpgradeRoom : public Room {
public:
    explicit UpgradeRoom(Coord c) : Room("Upgrade", c) {}
    void enter(Game* game) override;
};

class MonsterRoom : public Room {
public:
    explicit MonsterRoom(Coord c) : Room("Monster", c) {}
    void enter(Game* game) override;
};

class TreasureRoom : public Room {
public:
    explicit TreasureRoom(Coord c) : Room("Treasure", c) {}
    void enter(Game* game) override;
};

class ItemRoom : public Room {
public:
    explicit ItemRoom(Coord c) : Room("Item", c) {}
    void enter(Game* game) override;
};

// Monk attributes and methods
class Monk {
    std::string name;
    std::string description;
    int health;
    int maxHealth;
    int attack;
    int potions;

public:
    Monk(std::string n, std::string d) : name(n), description(d), health(15), maxHealth(15), attack(3), potions(0) {}

    [[nodiscard]] int getHealth() const { return health; }
    [[nodiscard]] int getAttack() const { return attack; }

    void takeDamage(int dmg) { health = std::max(0, health - dmg); }
    void heal(int amount) { health = std::min(maxHealth, health + amount); }
    void upgradeHealth() { maxHealth += 5; health = maxHealth; }
    void upgradeAttack() { attack += 2; }

    [[nodiscard]] bool isAlive() const { return health > 0; }
    [[nodiscard]] std::string getName() const { return name; }

    void addPotion() { ++potions; }
    bool usePotion() {
        if (potions > 0) {
            --potions;
            heal(5);
            std::cout << "You used a potion and healed 5 HP.\n";
            return true;
        }
        std::cout << "No potions left!\n";
        return false;
    }

    [[nodiscard]] int getPotionCount() const { return potions; }
};

//Enemy Interface and Types
class Enemy {
public:
    virtual ~Enemy() {}
    [[nodiscard]] virtual int getHealth() const = 0;
    [[nodiscard]] virtual int getAttack() const = 0;
    virtual void takeDamage(int dmg) = 0;
    virtual void heal(int amount) = 0;
    [[nodiscard]] virtual bool isAlive() const = 0;
    [[nodiscard]] virtual std::string getName() const = 0;
};

class Goblin : public Enemy {
    int health = 10;
    int attack = 2;
public:
    [[nodiscard]] int getHealth() const override { return health; }
    [[nodiscard]] int getAttack() const override { return attack; }
    void takeDamage(int dmg) override { health = std::max(0, health - dmg); }
    void heal(int amount) override { health = std::min(10, health + amount); }
    [[nodiscard]] bool isAlive() const override { return health > 0; }
    [[nodiscard]] std::string getName() const override { return "Goblin"; }
};

class Skeleton : public Enemy {
    int health = 6;
    int attack = 5;
public:
    [[nodiscard]] int getHealth() const override { return health; }
    [[nodiscard]] int getAttack() const override { return attack; }
    void takeDamage(int dmg) override { health = std::max(0, health - dmg); }
    void heal(int amount) override { health = std::min(6, health + amount); }
    [[nodiscard]] bool isAlive() const override { return health > 0; }
    std::string getName() const override { return "Skeleton"; }
};

class Boss : public Enemy {
    int health = 20;
    int attack = 7;
public:
    int getHealth() const override { return health; }
    int getAttack() const override { return attack; }
    void takeDamage(int dmg) override { health = std::max(0, health - dmg); }
    void heal(int amount) override { health = std::min(20, health + amount); }
    bool isAlive() const override { return health > 0; }
    std::string getName() const override { return "Dark Monk"; }
};

//Game States
class GameState {
public:
    virtual ~GameState() {}
    virtual void enter(Game* game) = 0;
};

class ExplorationState : public GameState {
public:
    void enter(Game* game) override;
};

class CombatState : public GameState {
public:
    void enter(Game* game) override;
};

class GameOverState : public GameState {
public:
    void enter(Game* game) override; // defined below Game, since it calls a Game method
};

//Dungeon- generation, storage and map printing
class Dungeon {
    std::vector<std::shared_ptr<Room>> rooms;

public:
    void addRoom(std::shared_ptr<Room> room) { rooms.push_back(room); }
    const std::vector<std::shared_ptr<Room>>& getRooms() const { return rooms; }

    Room* getRoomAt(Coord c) const {
        for (auto& r : rooms)
            if (r->getCoord() == c) return r.get();
        return nullptr;
    }

    void printMap(Room* currentRoom) {
        std::cout << "\nDungeon Map:\n";
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                Room* r = getRoomAt({x, y});
                if (r) {
                    char initial = r->getType()[0];
                    std::cout << "[" << (r == currentRoom ? initial + std::string("!") : std::string(1, initial)) << "] ";
                } else std::cout << "[ ] ";
            }
            std::cout << "\n";
        }
    }
};

//Game class- brings instances of the dungeon, main character, state and current room together
class Game {

    Dungeon dungeon;
    Monk monk;
    Room* currentRoom;
    std::shared_ptr<GameState> state;

public:
    Game(std::string name, std::string desc) : monk(name, desc), currentRoom(nullptr) {}

    void setCurrentRoom(Room* room) { currentRoom = room; }
    Room* getCurrentRoom() const { return currentRoom; }

    Dungeon* getDungeon() { return &dungeon; }
    Monk& getMonk() { return monk; }

    void setState(std::shared_ptr<GameState> s) { state = s; }
    void addRoom(std::shared_ptr<Room> room) { dungeon.addRoom(room); }

    void run() {
        while (state) {
            // Hold our own copy so the current state isn't destroyed
            // while its enter() is still running (enter() calls setState).
            std::shared_ptr<GameState> current = state;
            current->enter(this);
        }
    }

};

//Creates the games dungeon, handles random shuffling of rooms, and
class GameInitializer {
public:
    static void initializeGame(Game& game) {
        std::vector<Coord> coords;
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 3; ++x)
                coords.push_back({x, y});
        std::shuffle(coords.begin(), coords.end(), std::mt19937{std::random_device{}()});

        auto distance = [](const Coord& a, const Coord& b) {
            return std::abs(a.x - b.x) + std::abs(a.y - b.y);
        };

        // Start room: any cell except the centre. Nothing on a 3x3 grid is
        // 3+ steps from the centre, so a centre start could never place the treasure.
        auto startIt = std::find_if(coords.begin(), coords.end(), [](const Coord& c) {
            return !(c.x == 1 && c.y == 1);
        });
        Coord startCoord = *startIt;
        coords.erase(startIt);
        std::shared_ptr<Room> startRoom = std::make_shared<EmptyRoom>(startCoord);
        startRoom->markVisited(); // nothing happens in the room you start in

        // Treasure is placed BEFORE the other rooms, chosen only from cells at least
        // 3 steps away. A corner start has 3 such cells and an edge start has 2,
        // so one is always found (the old version could run off the end of the vector).
        auto treasureIt = std::find_if(coords.begin(), coords.end(), [&](const Coord& c) {
            return distance(c, startCoord) >= 3;
        });
        Coord treasureCoord = *treasureIt;
        coords.erase(treasureIt);

        std::vector<std::shared_ptr<Room>> specialRooms;
        specialRooms.push_back(std::make_shared<MonsterRoom>(coords.back())); coords.pop_back();
        specialRooms.push_back(std::make_shared<MonsterRoom>(coords.back())); coords.pop_back();
        specialRooms.push_back(std::make_shared<UpgradeRoom>(coords.back())); coords.pop_back();
        specialRooms.push_back(std::make_shared<UpgradeRoom>(coords.back())); coords.pop_back();
        specialRooms.push_back(std::make_shared<ItemRoom>(coords.back())); coords.pop_back();

        specialRooms.push_back(std::make_shared<TreasureRoom>(treasureCoord));

        std::vector<std::shared_ptr<Room>> allRooms = {startRoom};
        allRooms.insert(allRooms.end(), specialRooms.begin(), specialRooms.end());

        // The 2 cells left over become empty rooms, giving exactly 9 rooms
        for (auto& c : coords)
            allRooms.push_back(std::make_shared<EmptyRoom>(c));

        for (auto& r : allRooms) game.addRoom(r);

        for (auto& r1 : allRooms) {
            Coord c1 = r1->getCoord();
            for (auto& r2 : allRooms) {
                Coord c2 = r2->getCoord();
                if ((abs(c1.x - c2.x) == 1 && c1.y == c2.y) || (abs(c1.y - c2.y) == 1 && c1.x == c2.x)) {
                    r1->connect(r2.get());
                }
            }
        }

        game.setCurrentRoom(startRoom.get());
        game.setState(std::make_shared<ExplorationState>());
    }
};

void GameOverState::enter(Game* game) {
    std::cout << "\nGame Over!\n";
    game->setState(nullptr); // ends Game::run() normally instead of exit(0)
}

//Emtpy room heals and transitions back to map-
void EmptyRoom::enter(Game* game) {
    std::cout << "\nYou meditate and restore your health.\n";
    game->getMonk().heal(100);
    game->setState(std::make_shared<ExplorationState>());
}

void UpgradeRoom::enter(Game* game) {
    std::cout << "\nUpgrade Room! Choose an upgrade:\n1) +5 Max Health\n2) +2 Attack\nChoice: ";
    int choice = readChoice(1, 2);
    if (choice == 1) {
        game->getMonk().upgradeHealth();
        std::cout << "Max health increased by 5!\n";
    } else {
        game->getMonk().upgradeAttack();
        std::cout << "Attack increased by 2!\n";
    }
    game->setState(std::make_shared<ExplorationState>());
}

void MonsterRoom::enter(Game* game) {
    game->setState(std::make_shared<CombatState>());
}

void TreasureRoom::enter(Game* game) {
    std::cout << "\nYou approach the treasure, but a dark presence looms...\n";
    class BossCombatState : public GameState {
        void enter(Game* game) override {
            Monk& monk = game->getMonk();
            std::unique_ptr<Enemy> boss = std::make_unique<Boss>();

            std::cout << "\n Boss Fight Begins! \n";

            while (monk.isAlive() && boss->isAlive()) {
                std::cout << "\nMonk HP: " << monk.getHealth() << " | Potions: " << monk.getPotionCount()
                          << " | " << boss->getName() << " HP: " << boss->getHealth() << "\n";
                std::cout << "Choose action: 1) Attack  2) Guard  3) Use Potion: ";
                int action = readChoice(1, 3);

                bool success = rand() % 2;
                if (action == 1 && success) {
                    boss->takeDamage(monk.getAttack());
                    std::cout << "You struck the " << boss->getName() << "!\n";
                } else if (action == 2 && success) {
                    monk.heal(2);
                    std::cout << "You guarded and healed.\n";
                } else if (action == 3) {
                    monk.usePotion();
                } else {
                    std::cout << "Your action failed.\n";
                }

                if (!boss->isAlive()) break;

                success = rand() % 2;
                if (success) {
                    monk.takeDamage(boss->getAttack());
                    std::cout << boss->getName() << " slashes with his dark claws!\n";
                } else {
                    std::cout << boss->getName() << "'s attack missed.\n";
                }
            }

            if (monk.isAlive()) {
                std::cout << "\nYou defeated the Dark Monk and claimed the treasure! \n";
                game->setState(std::make_shared<GameOverState>());
            } else {
                std::cout << "\nYou were slain by the Dark Monk...\n";
                game->setState(std::make_shared<GameOverState>());
            }
        }
    };

    game->setState(std::make_shared<BossCombatState>());
}

void ItemRoom::enter(Game* game) {
    std::cout << "\nYou found a healing potion!\n";
    game->getMonk().addPotion();
    game->setState(std::make_shared<ExplorationState>());
}

void ExplorationState::enter(Game* game) {

    Room* currentRoom = game->getCurrentRoom();
    game->getDungeon()->printMap(currentRoom);

    std::cout << "\nYou are in a " << currentRoom->getType() << " room.\n";
    const auto& connections = currentRoom->getConnections();

    for (size_t i = 0; i < connections.size(); ++i) {
        Coord here = currentRoom->getCoord();
        Coord there = connections[i]->getCoord();
        std::string direction;
        if (there.x == here.x && there.y < here.y) direction = "North";
        else if (there.x == here.x && there.y > here.y) direction = "South";
        else if (there.y == here.y && there.x > here.x) direction = "East";
        else if (there.y == here.y && there.x < here.x) direction = "West";


        std::cout << i << ": [" << connections[i]->getType() << "] - " << direction << "\n";
    }

    std::cout << "Choose a room to enter (0-" << connections.size() - 1 << "): ";
    int choice = readChoice(0, static_cast<int>(connections.size()) - 1);

    Room* nextRoom = connections[choice];
    game->setCurrentRoom(nextRoom);

    // Each room's event only triggers on the first visit, so upgrades,
    // potions, heals and fights can't be farmed by walking back and forth.
    if (nextRoom->isVisited()) {
        std::cout << "\nYou have already cleared this " << nextRoom->getType() << " room.\n";
    } else {
        nextRoom->markVisited();
        nextRoom->enter(game);
    }
}

void CombatState::enter(Game* game) {
    Monk& monk = game->getMonk();

    std::unique_ptr<Enemy> enemy;
    if (rand() % 2)
        enemy = std::make_unique<Goblin>();
    else
        enemy = std::make_unique<Skeleton>();

    std::cout << "\nCombat with " << enemy->getName() << " begins!\n";

    while (monk.isAlive() && enemy->isAlive()) {
        std::cout << "\nMonk HP: " << monk.getHealth() << " | Potions: " << monk.getPotionCount()
                  << " | " << enemy->getName() << " HP: " << enemy->getHealth() << "\n";
        std::cout << "Choose action: 1) Attack  2) Guard  3) Use Potion: ";
        int action = readChoice(1, 3);

        bool success = rand() % 2;
        if (action == 1 && success) {
            enemy->takeDamage(monk.getAttack());
            std::cout << "You attacked the " << enemy->getName() << "!\n";
        } else if (action == 2 && success) {
            monk.heal(2);
            std::cout << "You guarded and healed 2 HP.\n";
        } else if (action == 3) {
            monk.usePotion();
        } else {
            std::cout << "Your action failed.\n";
        }

        if (!enemy->isAlive()) {

            if (dynamic_cast<Goblin*>(enemy.get())) {
                std::cout << " You feel energized from the Goblin. +3 HP!\n";
                monk.heal(3);
            }
            if (dynamic_cast<Skeleton*>(enemy.get())) {
                std::cout << " You learn from the Skeleton's form. +2 Attack!\n";
                monk.upgradeAttack();
            }

            break;
        }

        success = rand() % 2;
        int enemyAction = rand() % 2;
        if (enemyAction == 0 && success) {
            monk.takeDamage(enemy->getAttack() - enemy->getAttack() / 4);
            std::cout << enemy->getName() << " attacks you!\n";
        } else if (enemyAction == 1 && success) {
            enemy->heal(1);
            std::cout << enemy->getName() << " guards and heals.\n";

        } else {
            std::cout << enemy->getName() << "'s action failed.\n";
        }
    }

    if (monk.isAlive()) {
        std::cout << "You defeated the " << enemy->getName() << "!\n";
        game->setState(std::make_shared<ExplorationState>());
    } else {
        std::cout << "You were defeated...\n";
        game->setState(std::make_shared<GameOverState>());
    }
}

// ----------- Main Function -----------
int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    std::string name, desc;
    std::cout << "Enter Monk name: ";
    std::getline(std::cin, name);
    std::cout << "Enter Monk description: ";
    std::getline(std::cin, desc);

    Game game(name, desc);
    GameInitializer::initializeGame(game);
    game.run();

    // Keeps the console window open when the .exe is double-clicked
    std::cout << "\nPress Enter to exit.";
    std::string pause;
    std::getline(std::cin, pause);

    return 0;
}
