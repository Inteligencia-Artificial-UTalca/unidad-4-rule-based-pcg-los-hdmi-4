#include <iostream>
#include <vector>
#include <random>   // For random number generation
#include <chrono>   // For seeding the random number generator
#include <algorithm>

// Define Map as a vector of vectors of integers.
// You can change 'int' to whatever type best represents your cells (e.g., char, bool).
using Map = std::vector<std::vector<int>>;

/**
 * @brief Prints the map (matrix) to the console.
 * @param map The map to print.
 */
void printMap(const Map& map) {
    std::cout << "--- Current Map ---" << std::endl;
    for (const auto& row : map) {
        for (int cell : row) {
            // Adapt this to represent your cells meaningfully (e.g., ' ' for empty, '#' for occupied).
            std::cout << cell << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "-------------------" << std::endl;
}

/**
 * @brief Function to implement the Cellular Automata logic.
 * It should take a map and return the updated map after one iteration.
 * @param currentMap The map in its current state.
 * @param W Width of the map.
 * @param H Height of the map.
 * @param R Radius of the neighbor window (e.g., 1 for 3x3, 2 for 5x5).
 * @param U Threshold to decide if the current cell becomes 1 or 0.
 * @return The map after applying the cellular automata rules.
 */
Map cellularAutomata(const Map& currentMap, int W, int H, int R, double U) {
    Map newMap = currentMap; // Initially, the new map is a copy of the current one

    // TODO: IMPLEMENTATION GOES HERE for the cellular automata logic.
    // Iterate over each cell and apply the transition rules.
    // Remember that updates should be based on the 'currentMap' state
    // and applied to the 'newMap' to avoid race conditions within the same iteration.
    
    for (int i = 0; i < H; ++i) 
    {
        for (int j = 0; j < W; ++j) 
        {
            int count = 0;
            int total = 0;
            // Recorre la ventana de vecinos
            for (int dx = -R; dx <= R; ++dx) 
            {
                for (int dy = -R; dy <= R; ++dy) 
                {
                    int ni = i + dx;
                    int nj = j + dy;
                    if (ni >= 0 && ni < H && nj >= 0 && nj < W) 
                    {
                        count += currentMap[ni][nj];
                        total++;
                    } 
                }
            }
            
            double prop = static_cast<double>(count) / total;
            newMap[i][j] = (prop > U) ? 1 : 0;
        }
    }
    return newMap;
}

/**
 * @brief Function to implement the Drunk Agent logic.
 * It should take a map and parameters controlling the agent's behavior,
 * then return the updated map after the agent performs its actions.
 *
 * @param currentMap The map in its current state.
 * @param W Width of the map.
 * @param H Height of the map.
 * @param J The number of times the agent "walks" (initiates a path).
 * @param I The number of steps the agent takes per "walk".
 * @param roomSizeX Max width of rooms the agent can generate.
 * @param roomSizeY Max height of rooms the agent can generate.
 * @param probGenerateRoom Probability (0.0 to 1.0) of generating a room at each step.
 * @param probIncreaseRoom If no room is generated, this value increases probGenerateRoom.
 * @param probChangeDirection Probability (0.0 to 1.0) of changing direction at each step.
 * @param probIncreaseChange If direction is not changed, this value increases probChangeDirection.
 * @param agentX Current X position of the agent (updated by reference).
 * @param agentY Current Y position of the agent (updated by reference).
 * @return The map after the agent's movements and actions.
 */
Map drunkAgent(const Map& currentMap, int W, int H, int J, int I, int roomSizeX, int roomSizeY,
               double probGenerateRoom, double probIncreaseRoom,
               double probChangeDirection, double probIncreaseChange,
               int& agentX, int& agentY) {
    Map newMap = currentMap; // The new map is a copy of the current one

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dirDist(0, 3);
    std::uniform_real_distribution<> probDist(0.0, 1.0);

    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {-1, 0, 1, 0};
    int direction = dirDist(gen);

    for(int j = 0; j < J; j++)
    {
       for(int i = 0; i < I; i++)
       {

        if(agentX >= 0 && agentX < H && agentY >= 0 && agentY < W)
        {
           newMap[agentX][agentY] = 1;
        }

        //proba para generar una pieza

        if(probDist(gen) < probGenerateRoom)
        {
          // Genera la habitacion centrada en el agente
            int startX = std::max(0, agentX - roomSizeX / 2);
            int endX = std::min(H, agentX + roomSizeX / 2);
            int startY = std::max(0, agentY - roomSizeY / 2);
            int endY = std::min(W, agentY + roomSizeY / 2);
             
             for (int x = startX; x < endX; ++x)
                    for (int y = startY; y < endY; ++y)
                        newMap[x][y] = 1;

                probGenerateRoom = probIncreaseRoom; // Reinicia la probaAAAAAAAAAAAAA
            } else 
            {
                probGenerateRoom += probIncreaseRoom; // Aumenta la probaAAAAAAAAAAAA
            }

            // Probabilidad de cambiar la direccion
            if (probDist(gen) < probChangeDirection) 
            {
                direction = dirDist(gen);
                probChangeDirection = probIncreaseChange;
            } else 
            {
                probChangeDirection += probIncreaseChange;
            }

            // Mueve al agente
            int nextX = agentX + dx[direction];
            int nextY = agentY + dy[direction];

            // Si se sale del borde, cambia la direccion al azar 
            if (nextX < 0 || nextX >= H || nextY < 0 || nextY >= W) 
            {
                direction = dirDist(gen);
                break;
            } else 
            {
                agentX = nextX;
                agentY = nextY;
            }
            
        }

       }
    
    return newMap;
}

int main() {
     std::cout << "--- CELLULAR AUTOMATA AND DRUNK AGENT SIMULATION ---" << std::endl;

    // --- Configuración inicial del mapa ---
    int mapRows = 10;
    int mapCols = 20;
    Map myMap(mapRows, std::vector<int>(mapCols, 0)); 

    //el bool es para cambiar de agente
    bool usarAutomataCelular = false;
    bool usarDrunkAgent = true; 

    // --- Inicialización aleatoria para autómata celular ---
    if (usarAutomataCelular) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> valDist(0, 1);
        for (int i = 0; i < mapRows; ++i)
            for (int j = 0; j < mapCols; ++j)
                myMap[i][j] = valDist(gen);
    }
   

    // --- Posición inicial del Drunk Agent ---
    int drunkAgentX = mapRows / 2;
    int drunkAgentY = mapCols / 2;

    std::cout << "\nEstado inicial del mapa:" << std::endl;
    printMap(myMap);

    // --- Parámetros de la simulación ---
    int numIterations = 5; // Número de iteraciones

    // Parámetros Cellular Automata
    int ca_W = mapCols;
    int ca_H = mapRows;
    int ca_R = 1;      // Radio de la ventana de vecinos
    double ca_U = 0.5; // Umbral

    // Parámetros Drunk Agent
    int da_W = mapCols;
    int da_H = mapRows;
    int da_J = 5;      // Caminatas
    int da_I = 10;     // Pasos por caminata
    int da_roomSizeX = 5;
    int da_roomSizeY = 3;
    double da_probGenerateRoom = 0.1;
    double da_probIncreaseRoom = 0.05;
    double da_probChangeDirection = 0.2;
    double da_probIncreaseChange = 0.03;

    // --- Bucle principal de la simulación ---
    for (int iteration = 0; iteration < numIterations; ++iteration) {
        std::cout << "\n--- Iteración " << iteration + 1 << " ---" << std::endl;

        if (usarAutomataCelular) {
            myMap = cellularAutomata(myMap, ca_W, ca_H, ca_R, ca_U);
        }

        if (usarDrunkAgent) {
            myMap = drunkAgent(myMap, da_W, da_H, da_J, da_I, da_roomSizeX, da_roomSizeY,
                               da_probGenerateRoom, da_probIncreaseRoom,
                               da_probChangeDirection, da_probIncreaseChange,
                               drunkAgentX, drunkAgentY);
        }

        printMap(myMap);
    }

    std::cout << "\n--- Simulación terminada ---" << std::endl;
    return 0;
}