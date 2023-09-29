#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 100;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075; //
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025; //
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

// Função para criar entidades aleatoriamente na grade
void createEntities(uint32_t numPlants, uint32_t numHerbivores, uint32_t numCarnivores)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> randomRow(0, NUM_ROWS - 1);
    std::uniform_int_distribution<int> randomCol(0, NUM_ROWS - 1);

    // Criar plantas
    for (uint32_t i = 0; i < numPlants; ++i)
    {
        int row, col;
        do
        {
            row = randomRow(gen);
            col = randomCol(gen);
        } while (entity_grid[row][col].type != empty);

        entity_grid[row][col] = {plant, MAXIMUM_ENERGY, 0};
    }

    // Criar herbívoros
    for (uint32_t i = 0; i < numHerbivores; ++i)
    {
        int row, col;
        do
        {
            row = randomRow(gen);
            col = randomCol(gen);
        } while (entity_grid[row][col].type != empty);

        entity_grid[row][col] = {herbivore, MAXIMUM_ENERGY, 0};
    }

    // Criar carnívoros
    for (uint32_t i = 0; i < numCarnivores; ++i)
    {
        int row, col;
        do
        {
            row = randomRow(gen);
            col = randomCol(gen);
        } while (entity_grid[row][col].type != empty);

        entity_grid[row][col] = {carnivore, MAXIMUM_ENERGY, 0};
    }
}

static std::random_device rd;
static std::mt19937 gen(rd());

bool random_action(float probability)
{
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
            res.code = 400;
            res.body = "Too many entities";
            res.end();
            return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Dentro do endpoint "/start-simulation"
        createEntities((uint32_t)request_body["plants"], (uint32_t)request_body["herbivores"], (uint32_t)request_body["carnivores"]);
    
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration").methods("GET"_method)([]()
                                                             {
        // Simular a próxima iteração
 
        // Loop through the entity grid and simulate behavior
        for (uint32_t row = 0; row < NUM_ROWS; ++row) {
            for (uint32_t col = 0; col < NUM_ROWS; ++col) {
                entity_t &current_entity = entity_grid[row][col];
                current_entity.age++;
                
                if (current_entity.type == empty) {
                    // Ignore empty cells
                    continue;
                } else if (current_entity.type == plant) {
                   
                    if (random_action(PLANT_REPRODUCTION_PROBABILITY)){
                        std::uniform_int_distribution<int> randomMove(1, 4); 
                        int newRow, newCol;
                        do {
                            int moveDirection = randomMove(gen);
                            if (moveDirection == 1) {
                                newRow = row + 1;
                                newCol = col;
                            } else if (moveDirection == 2) {
                                newRow = row - 1;
                                newCol = col;
                            } else if (moveDirection == 3) {
                                newRow = row;
                                newCol = col + 1;
                            } else if (moveDirection == 4) {
                                newRow = row;
                                newCol = col - 1;
                            }
                            
                        } while (newRow < 0 || newRow >= NUM_ROWS || newCol < 0 || newCol >= NUM_ROWS);

    
                        // Check if the new position is empty (not occupied by another entity)
                        if (entity_grid[newRow][newCol].type == empty) {
                            entity_grid[newRow][newCol] = {plant, MAXIMUM_ENERGY, 0};
                        }
                    }

                    // Plants age and die
                    if (current_entity.age >= PLANT_MAXIMUM_AGE) {
                        current_entity.type = empty;
                        current_entity.energy = 0;
                        current_entity.age = 0;
                    }
                } else if (current_entity.type == herbivore) {
                    
                    // Herbivores move
                    std::uniform_int_distribution<int> randomMove(1, 4);
                    if (random_action(HERBIVORE_MOVE_PROBABILITY)) {

                        int newRow, newCol;
                        do {
                            int moveDirection = randomMove(gen);
                            if (moveDirection == 1) {
                                newRow = row + 1;
                                newCol = col;
                            } else if (moveDirection == 2) {
                                newRow = row - 1;
                                newCol = col;
                            } else if (moveDirection == 3) {
                                newRow = row;
                                newCol = col + 1;
                            } else if (moveDirection == 4) {
                                newRow = row;
                                newCol = col - 1;
                            }
                        } while (newRow < 0 || newRow >= NUM_ROWS || newCol < 0 || newCol >= NUM_ROWS);

                        // Check if the new position is empty (not occupied by another entity)
                        if (entity_grid[newRow][newCol].type == empty) {
                            // Move the herbivore: update its position
                            current_entity.energy -= 5;
                            entity_grid[newRow][newCol] = current_entity;
                            entity_grid[row][col] = {empty, 0, 0}; // Clear the previous position
                        }
                    }

                // Herbivore eating logic
                    else if (current_entity.energy > 0) { // O herbívoro só pode comer se tiver energia positiva
                        if((row + 1) < NUM_ROWS) {
                            if(entity_grid[row+1][col].type == plant){
                                if (random_action(HERBIVORE_EAT_PROBABILITY)) {
                                    current_entity.energy += 30;
                                    entity_grid[row+1][col].type = empty;
                                    entity_grid[row+1][col].age = 0;
                                    entity_grid[row+1][col].energy = 0;                                }
                            }
                        }
                        if((row) > 0) {
                            if(entity_grid[row-1][col].type == plant){
                                if (random_action(HERBIVORE_EAT_PROBABILITY)) {
                                    current_entity.energy += 30;
                                    entity_grid[row-1][col].type = empty;
                                    entity_grid[row-1][col].age = 0;
                                    entity_grid[row-1][col].energy = 0;
                                }
                            }
                        }
                        if((col + 1) < NUM_ROWS){
                            if(entity_grid[row][col+1].type == plant){
                                if (random_action(HERBIVORE_EAT_PROBABILITY)) {
                                    current_entity.energy += 30;
                                    entity_grid[row][col+1].type = empty;
                                    entity_grid[row][col+1].age = 0;
                                    entity_grid[row][col+1].energy = 0;
                                }
                            }
                        }
                        
                        if((col) > 0){
                            if(entity_grid[row][col-1].type == plant){
                                if (random_action(HERBIVORE_EAT_PROBABILITY)) {
                                    current_entity.energy += 30;
                                    entity_grid[row][col-1].type = empty;
                                    entity_grid[row][col-1].age = 0;
                                    entity_grid[row][col-1].energy = 0;
                                }
                            }
                        }

                    }
                                    
                    // Herbivores reproduce
                    else if (random_action(HERBIVORE_REPRODUCTION_PROBABILITY) &&
                        current_entity.energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION) {
                        
                        int newRow = row;
                        int newCol = col;
                        do {
                            int moveDirection = randomMove(gen);
                            if (moveDirection == 1) {
                                newRow = row + 1;
                            } else if (moveDirection == 2) {
                                newRow = row - 1;
                            } else if (moveDirection == 3) {
                                newCol = col + 1;
                            } else if (moveDirection == 4) {
                                newCol = col - 1;
                            }
                            
                        } while (newRow < 0 || newRow >= NUM_ROWS || newCol < 0 || newCol >= NUM_ROWS);

    
                        // Check if the new position is empty (not occupied by another entity)
                        if (entity_grid[newRow][newCol].type == empty) {
                            entity_grid[newRow][newCol] = {herbivore, MAXIMUM_ENERGY, 0};
                            current_entity.energy -= 10;
                        }
                    }

                    // Herbivores age and die
                    
                    if (current_entity.age >= HERBIVORE_MAXIMUM_AGE || current_entity.energy <= 0) {
                        current_entity.type = empty;
                        current_entity.energy = 0;
                        current_entity.age = 0;
                    }
                    
                } else if (current_entity.type == carnivore) {
                    // Carnivores move
                    if (random_action(CARNIVORE_MOVE_PROBABILITY)) {

                        std::uniform_int_distribution<int> randomMove(1, 4); 
                        int newRow, newCol;
                        do {
                            int moveDirection = randomMove(gen);
                            if (moveDirection == 1) {
                                newRow = row + 1;
                                newCol = col;
                            } else if (moveDirection == 2) {
                                newRow = row - 1;
                                newCol = col;
                            } else if (moveDirection == 3) {
                                newRow = row;
                                newCol = col + 1;
                            } else if (moveDirection == 4) {
                                newRow = row;
                                newCol = col - 1;
                            }
                        } while (newRow < 0 || newRow >= NUM_ROWS || newCol < 0 || newCol >= NUM_ROWS);

                        // Check if the new position is empty (not occupied by another entity)
                        if (entity_grid[newRow][newCol].type == empty) {
                            // Move the carnivore: update its position
                            current_entity.energy -= 5;
                            entity_grid[newRow][newCol] = current_entity;
                            entity_grid[row][col] = {empty, 0, 0}; // Clear the previous position
                        }
                    }

                    // Carnivores eat herbivores
                    if (current_entity.energy > 0) { // O herbívoro só pode comer se tiver energia positiva
                        if((row + 1) < NUM_ROWS) {
                            if(entity_grid[row+1][col].type == herbivore){
                                    current_entity.energy += 20;
                                    entity_grid[row+1][col].type = empty;
                                    entity_grid[row+1][col].age = 0;
                                    entity_grid[row+1][col].energy = 0;                                
                        }
                        if((row) > 0) {
                            if(entity_grid[row-1][col].type == herbivore){
                                    current_entity.energy += 20;
                                    entity_grid[row-1][col].type = empty;
                                    entity_grid[row-1][col].age = 0;
                                    entity_grid[row-1][col].energy = 0;
                            }
                        }
                        if((col + 1) < NUM_ROWS){
                            if(entity_grid[row][col+1].type == herbivore){
                                    current_entity.energy += 20;
                                    entity_grid[row][col+1].type = empty;
                                    entity_grid[row][col+1].age = 0;
                                    entity_grid[row][col+1].energy = 0;
                            }
                        }
                        
                        if((col) > 0){
                            if(entity_grid[row][col-1].type == herbivore){
                                    current_entity.energy += 20;
                                    entity_grid[row][col-1].type = empty;
                                    entity_grid[row][col-1].age = 0;
                                    entity_grid[row][col-1].energy = 0;
                            }
                        }

                    }
                    

                    // Carnivores reproduce
                    if (random_action(CARNIVORE_REPRODUCTION_PROBABILITY) &&
                        current_entity.energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION) {
                        std::uniform_int_distribution<int> randomMove(1, 4); 
                        int newRow, newCol;
                        do {
                            int moveDirection = randomMove(gen);
                            if (moveDirection == 1) {
                                newRow = row + 1;
                                newCol = col;
                            } else if (moveDirection == 2) {
                                newRow = row - 1;
                                newCol = col;
                            } else if (moveDirection == 3) {
                                newRow = row;
                                newCol = col + 1;
                            } else if (moveDirection == 4) {
                                newRow = row;
                                newCol = col - 1;
                            }
                            
                        } while (newRow < 0 || newRow >= NUM_ROWS || newCol < 0 || newCol >= NUM_ROWS);

    
                        // Check if the new position is empty (not occupied by another entity)
                        if (entity_grid[newRow][newCol].type == empty) {
                            entity_grid[newRow][newCol] = {carnivore, MAXIMUM_ENERGY, 0};
                            current_entity.energy -= 10;
                        }
                    }

                    // Carnivores age and die
                    if (current_entity.age >= CARNIVORE_MAXIMUM_AGE || current_entity.energy <= 0) {
                        current_entity.type = empty;
                        current_entity.energy = 0;
                        current_entity.age = 0;
                    }
                    }
                }
            }
        }


        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8084).run();

    return 0;
}