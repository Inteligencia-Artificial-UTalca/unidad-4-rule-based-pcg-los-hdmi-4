#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>
#include <string>
#include <limits>
#include <chrono>
#include "Perlin_Noise.h"
#include "Enemy.h"
//Para leer teclas
#include <termios.h>
#include <unistd.h> //no existe en windows pero si en linux
//Si compila esto



//Settings del mapa
int WIDTH = 40;
int HEIGHT = 20;
int MIN_ROOM_SIZE = 4;
int MAX_ROOM_SIZE = 8;
int MIN_LEAF_SIZE = 8;
int PASILLO_LOGICA = 0; //o es azar, 1 horizontal primero y el 2 es vertical primero
int ENEMIGOS_HABITACION = 5;

//Parametros de PerlinNoise (No implementado)
double FREQ = 0.08;
double THRES = 0.5;
double MAXSPAWN = 0.3; //30% 
int SECURITYRATIO = 3; //Area segura alrededor del jugador y la salida

// Variables de caractéres
const char floor_char = '.'; //Camino transitable
const char wall_char = '#'; // Muro
const char empty_char = '0'; //Espacio vacio 
const char enemy_char = 'E'; //Enemigo
const char player_char = 'P'; //el player
const char exit_char = 'S'; //La salida
const char door_char = 'D'; // A futuro

//Representa una sala rectangular en el mapa
struct Room 
{
  int x, y; //Posiciones 
  int w, h; //Ancho y alto
  int center_x() const {return x + w /2; } //centro horizontal
  int center_y() const {return y + h /2;} //centro vertical
};

//Nodo del arbol BSP, cada hoja se puede subdividir de 2 en 2
struct Leaf
{
  int x, y, w, h; //dimensiones del area
  Leaf* left = nullptr; //hoja hija izquierda
  Leaf* right = nullptr; //hoja hija derecha
  Room* room = nullptr; // es la room que almacena en la hoja

  Leaf(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    ~Leaf() { delete left; delete right; delete room; }
};

char getch() 
{
    char buf = 0;
    struct termios old = {};
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
    struct termios newt = old;
    newt.c_lflag &= ~ICANON; // Modo sin buffer
    newt.c_lflag &= ~ECHO;   // No mostrar caracter
    if (tcsetattr(0, TCSANOW, &newt) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror ("read()");
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror ("tcsetattr ~ICANON");
    return buf;
}
//=======================RNG global======================//
std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));

//==================Divide la "hoja" en dos partes que son hijas===============//
bool Split(Leaf* leaf)
{
  //Divide la hoja en dos partes segun la orientacion ya sea horizontal o vertical
  //Si logra dividir devuelve un true y por el contrario un false
  if(leaf ->left || leaf ->right)
  {
     return false;//Significa que ya fue divido antes
  }
  
  //Permite determinar la orientacion de la división
  bool splitH = (leaf->w > leaf->h);
  if(leaf->w / leaf->h >= 1.25)
  {
    splitH = false;
  }
  else if(leaf->h / leaf->w >= 1.25)
  {
   splitH = true;
  }
   
  //Elige el punto de corte (al azar) siempre que esté dentro del rango permitido
  std::uniform_int_distribution<int> split_dist(MIN_LEAF_SIZE, (splitH ? leaf->h : leaf->w) - MIN_LEAF_SIZE);

  //Esto es para que cuando no haya suficiento espacio no siga dividiendo
  if((splitH ? leaf->h : leaf->w) < MIN_LEAF_SIZE * 2)
  {
    return false;
  }

  int split_point = split_dist(rng);
  if(splitH)
  {
   leaf->left = new Leaf(leaf->x, leaf->y, leaf->w, split_point);
   leaf->right = new Leaf(leaf->x, leaf->y + split_point, leaf->w, leaf->h - split_point);
  }
  else
  {
   leaf->left = new Leaf(leaf->x, leaf->y, split_point, leaf->h);
   leaf->right = new Leaf(leaf->x + split_point, leaf->y, leaf->w - split_point, leaf->h);
  }

  return true;   
}

//====================DIVIDE DE FORMA RECURSIVA LAS HOJAS===============//

void SplitAllLeaves(std::vector<Leaf*>& leaves)
{
  //Divide de forma recursiva las hojas en la lista
  //Hasta que no pueda dividir mas de acuerdo al MIN_LEAF_SIZE
  bool did_split;

  do
  {
   did_split = false;
   std::vector<Leaf*> new_leaves;
   for(auto leaf : leaves)
   {
     if(!leaf -> left && !leaf->right)
     {
      if(leaf ->w > MIN_LEAF_SIZE * 2 || leaf->h > MIN_LEAF_SIZE * 2)
      {
       if(Split(leaf))
       {
         new_leaves.push_back(leaf->left);
         new_leaves.push_back(leaf->right);
         did_split = true;
       }
      }

     }


   }
 
    //Añade mas hojas a la lista para el siguiente ciclo
    for(auto l : new_leaves) leaves.push_back(l);
  }while(did_split);
}

//==============GENERADOR DE ROOM CON LAS HOJAS DEL ARBOL============//
void CreateRoom(Leaf* leaf)
{
  //Crea la sala rectangular dentro de la hoja
  //siempre dentro de los limites establecidos
 if(leaf->left || leaf->right)
 {
   if(leaf->left)
   {
    CreateRoom(leaf->left);
   }
   if(leaf->right)
   {
    CreateRoom(leaf->right);
   }  
 }
 else
 {
  //Calcula el tamaño maximo que permite la sala según la hoja 
  int max_room_w = std::min(MAX_ROOM_SIZE, leaf->w - 2);
  int max_room_h = std::min(MAX_ROOM_SIZE, leaf->h - 2);
  if (MIN_ROOM_SIZE > max_room_w || MIN_ROOM_SIZE > max_room_h)
  {
     //std::cout << "ERROR: Medidas no validas\n";
     return;
  }
           
  //std::uniform_int_distribution<int> room_w_dist(MIN_ROOM_SIZE, leaf->w - 2);
  //std::uniform_int_distribution<int> room_h_dist(MIN_ROOM_SIZE, leaf->h - 2);
  //Para usar el tamaño maximo de la room
  std::uniform_int_distribution<int> room_w_dist(MIN_ROOM_SIZE, std::min(MAX_ROOM_SIZE, leaf->w - 2));
  std::uniform_int_distribution<int> room_h_dist(MIN_ROOM_SIZE, std::min(MAX_ROOM_SIZE, leaf->h - 2));
  int rw = std::min(room_w_dist(rng), leaf->w - 2);
  int rh = std::min(room_h_dist(rng), leaf->h - 2);
  std::uniform_int_distribution<int> room_x_dist(leaf->x + 1, leaf->x + leaf->w - rw - 1);
  std::uniform_int_distribution<int> room_y_dist(leaf->y + 1, leaf->y + leaf->h - rh - 1);

  int rx = room_x_dist(rng);
  int ry = room_y_dist(rng);
  leaf->room = new Room{ rx, ry, rw, rh };
 }
}

//===================DIBUJA LAS ROOMS GENERADAS Y GUARDA SUS PUNTEROS==========//
void FillRoom(std::vector<std::vector<char>>& map, Leaf* leaf, std::vector<Room*>& rooms)
{
  //Completa el mapa con las salas que genero, usando el floor_char como piso
  //almacena los punteros de las salas en la lista de rooms
  if (leaf->left || leaf->right)
  {  
    if (leaf->left)
    {
      FillRoom(map, leaf->left, rooms);
    } 
    if (leaf->right)
    {
      FillRoom(map, leaf->right, rooms);
    } 
  }
  else if (leaf->room)
  {
    rooms.push_back(leaf->room);
    for (int y = leaf->room->y; y < leaf->room->y + leaf->room->h; ++y)
    {
     for (int x = leaf->room->x; x < leaf->room->x + leaf->room->w; ++x)
     {
        map[y][x] = floor_char;
     }           
    }
        
  }
}

//====================CONECTA LOS CENTROS DE LAS ROOM HACIENDO UN PASILLO=========//
void ConnectRooms(std::vector<std::vector<char>>& map, Room* a, Room* b)
{
  //conecta las salas dibujando un pasillo segun la configuracion
  
 int x1 = a->center_x(), y1 = a->center_y();
 int x2 = b->center_x(), y2 = b->center_y();

 if(PASILLO_LOGICA == 0)
 {
  if(rng() %2)
   {
     //el horizontal primero
     for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
     {
        map[y1][x] = floor_char;
     }
     //el vertical despues
     for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
     {
        map[y][x2] = floor_char;
     } 
   }
  else
   {
    //vertival primero
     for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
     {
        map[y][x1] = floor_char;
     } 
     //horizontal despues
     for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
     {
        map[y2][x] = floor_char;
     } 
   }
 }

 else if(PASILLO_LOGICA == 1)
 {
   for(int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
   {
      map[y1][x] = floor_char;
   }
   for(int y = std::min(y1, y2); y <= std::max(y1, y2); y++)
   {
      map[y][x2] = floor_char;
   } 
 }

 else if(PASILLO_LOGICA == 2)
 {
   for(int y = std::min(y1, y2); y <= std::max(y1, y2); y++)
   {
      map[y][x1] = floor_char;
   }
   for(int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
   {
      map[y2][x] = floor_char;
   } 
 }

 else if(PASILLO_LOGICA == 3)
 {
   if(abs(x1 - x2) >= abs(y1 - y2))//Horizontal
   {
     for(int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
     {
       map[y1][x] = floor_char;
     } 
   }
   else//Vertical
   {
     for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
     {
       map[y][x1] = floor_char;
     } 
   }
 }
 
}

//===============CONECTA LAS ROOMS EN EL ARBOL BSP========//
void ConnectLeafRoom(std::vector<std::vector<char>>& map, Leaf* leaf, int& conexiones)
{
  //recorre el arbol bsp conectando las sals entre si
  //por cada nodo con dos hijos los conecta de izquierda a derecha
 if(leaf->left && leaf->right)
 {
  Room* a = nullptr;
  Room* b = nullptr;
  // Busca una habitacion en cada subarbol
  std::vector<Leaf*> stack;
  stack.push_back(leaf->left);
  while (!stack.empty()) 
  {
     Leaf* l = stack.back(); stack.pop_back();
     if (l->room) 
     { 
       a = l->room; break; 
     }

     if (l->left)
     {
       stack.push_back(l->left);
     } 

     if (l->right)
     {
       stack.push_back(l->right);
     } 
  }

  stack.push_back(leaf->right);
  while(!stack.empty())
  {
    Leaf* l = stack.back(); stack.pop_back();
    if (l->room)
    { 
      b = l->room; break; 
    }

    if (l->left)
    {
      stack.push_back(l->left);
    } 

    if (l->right)
    {
      stack.push_back(l->right);
    } 
  }
   if(a && b)
   {
     ConnectRooms(map, a, b);
     conexiones++;
   }  
   //lo recorre recursivamente para ambos hijos
   ConnectLeafRoom(map, leaf->left, conexiones);
   ConnectLeafRoom(map, leaf->right, conexiones);
 }
}

//===============SPAWn DEL PLAYER EN LA ROOM 1==========//
void PlacePlayer(std::vector<std::vector<char>>& map, std::vector<Room*>& rooms) 
{
    if (!rooms.empty()) 
    {
        map[rooms[0]->center_y()][rooms[0]->center_x()] = player_char;
    }
}

//================GENERAR ENEMIGOS==============//
void PlacedEnemy(std::vector<std::vector<char>>& map, std::vector<Room*>& rooms, int enemigos_por_habitacion)
{
  for(size_t i = 1; i < rooms.size(); ++i)
  {
     Room* r = rooms[i];
     int intentos = 0; //en caso de tener una room muy pequeña
     int max_intentos = 10 * enemigos_por_habitacion;
     int enemigos_colocados = 0;

     while(enemigos_colocados < enemigos_por_habitacion && intentos < max_intentos)
     {
        std::uniform_int_distribution<int> posx(r->x, r->x + r->w - 1);
        std::uniform_int_distribution<int> posy(r->y, r->y + r->h - 1);
        int ex = posx(rng);
        int ey = posy(rng);
        //Spawnear el enemigo en una parte libre sin muro

        if(map[ey][ex] == floor_char)
        {
          map[ey][ex] = enemy_char;
          enemigos_colocados++;
        }
        intentos++;
     }
  }
}


//===================GENERAR UNA SALIDA==================//

void PlaceExit(std::vector<std::vector<char>>& map, std::vector<Room*>& rooms)
{
  if (rooms.empty()) return;
    int px = rooms[0]->center_x();
    int py = rooms[0]->center_y();

    int max_dist = -1;
    size_t idx_salida = 0;

    // Recorre todas las salas menos la inicial
    for (size_t i = 1; i < rooms.size(); ++i) 
    {
        int cx = rooms[i]->center_x();
        int cy = rooms[i]->center_y();
        int dist = abs(cx - px) + abs(cy - py); // aplica distancia de Manhattan
        if (dist > max_dist) 
        {
            max_dist = dist;
            idx_salida = i;
        }
    }

    // Coloca la S en la room mas lejana, en un punto que no tenga muro
    Room* salida = rooms[idx_salida];
    int sx = salida->center_x();
    int sy = salida->center_y();

    // Si hay un enemigo en el centro, busca otra parte
    if (map[sy][sx] == floor_char) 
    {
        map[sy][sx] = exit_char;
    } else 
    {
        // Busca cualquier punto libre dentro de la sala
        for (int y = salida->y; y < salida->y + salida->h; ++y) 
        {
            for (int x = salida->x; x < salida->x + salida->w; ++x) 
            {
                if (map[y][x] == floor_char) 
                {
                    map[y][x] = exit_char;
                    return;
                }
            }
        }
    }
}

//==============MOSTRAR EL MAPA==========//
void PrintMap(const std::vector<std::vector<char>>& map)
{
 for (const auto& row : map)
 { 
    for (char c : row)
    {
        std::cout << c;
        
    }     
    std::cout << "\n";   
 }
}

//====================PARAMETRIZAR EL MAPA=============//
int LeerParametro(const std::string& prompt, int valor_por_defecto, int minimo)
{
  //Valida la parametrizacion del mapa
  //recibe el valor por defecto que es la variable declara
  //recibe un minimo asignado por el codigo en el main
  //y compara para evitar que el usuario ingrese un valor por debajo del minimo
 std::string input;
 int valor;

 while(true)
 {
   std::cout << prompt << " (por defecto: " << valor_por_defecto << ", minimo: " << minimo << "): ";
   std::getline(std::cin, input);

   if(input.empty())
   {
    valor = valor_por_defecto;
    break;
   }

   try
   {
    valor = std::stoi(input);
    if(valor < minimo)
    {
     std::cout << "El valor debe ser al menos " << minimo << ".\n";
    }
    else
    {
      break;
    }
   }
   catch(const std::exception& e)
   {
    std::cout << "Ingrese un valor soportado || Presione Enter en blanco para usar el valor por defecto.\n";
   }
   
 }
 return valor;
}

int main()
{
    int opcion = 0;
    bool ciclo = true;

  do
  {    
    
    //Parametros de la particion
    std::cout << "===Parametros de la particion\n";
    MIN_LEAF_SIZE = LeerParametro("Medida minima de la particion de la hoja:\n",MIN_LEAF_SIZE,4);

    // Calcula el minimo requerido para que funcione el BSP
    int minimo_tamano = MIN_LEAF_SIZE * 2 + 2;
  
     // Pide al usuario los parametros
    std::cout << "=== Parametros del Mapa ===\n";
    WIDTH = LeerParametro("Ancho (WIDTH)\n", WIDTH, minimo_tamano);
    std::cout << "\n";
    HEIGHT = LeerParametro("Alto (HEIGHT)\n", HEIGHT, minimo_tamano);
    
    std::cout << "===Rango de tamaño de las salas===\n";
    MIN_ROOM_SIZE = LeerParametro(" Medida minima de la sala :\n", MIN_ROOM_SIZE, 2);
    MAX_ROOM_SIZE = LeerParametro(" Medida maxima de la sala :\n", MAX_ROOM_SIZE, 4);

    std::cout << "===Conexiones de los pasillos===\n";
    std::cout << "[0]. Azar | [1]. Horizontal | [2]. Vertical | [3]. Recto\n";
    PASILLO_LOGICA = LeerParametro(" Ingrese la opcion de conexion :", PASILLO_LOGICA, 0);//esto se nota al generar rooms grandes
    if(  PASILLO_LOGICA < 0 || PASILLO_LOGICA > 3)
    {
      PASILLO_LOGICA = 0;
    }

    std::cout << "===Parametros de enemigos===\n";
       ENEMIGOS_HABITACION = LeerParametro("Cantidad de enemigos por habitación", ENEMIGOS_HABITACION, 0); 
       //No funciono el Perlin Noise :c
      //FREQ = LeerParametro("Frecuencia de Perlin  | Por defecto: 0.08) : \n", 0.08, 0.01);
      //THRES = LeerParametro("Umbral de enemigos Perlin  | Por defecto: 0.5)\n", 0.5, 0.0);
      //MAXSPAWN = LeerParametro("Porcentaje maximo de spawn de enemigos | Por defecto 0.3 (30%) :\n", 0.3, 0);
      //SECURITYRATIO = LeerParametro("Area segura entorno al Jugador/Salida | Por defecto 3 :\n", 3, 0);

    std::cout << "Presiona ENTER para generar un nuevo mapa, ESC para salir...\n";
    while (true) {

        //Estas variables deben reiniciarse
        int conexiones = 0;
        int enemigos_total = 0;
        std::cout << "\033[2J\033[1;1H"; //Esto es para leer las teclas como esc
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::vector<char>> map(HEIGHT, std::vector<char>(WIDTH, wall_char));
        Leaf* root = new Leaf(0, 0, WIDTH, HEIGHT);

        std::vector<Leaf*> leaves = { root };
        SplitAllLeaves(leaves);
        CreateRoom(root);

        std::vector<Room*> rooms;
        FillRoom(map, root, rooms);

        ConnectLeafRoom(map, root,conexiones);
        PlacePlayer(map, rooms);
        PlacedEnemy(map, rooms, ENEMIGOS_HABITACION); 
        
        //0.1 es la frecuencia de enemigos agrupados en las zonas de mayor tamaño
        //0.6 es el umbral de ruido donde aparece un enemigos si es mayor 
       // PlacedEnemyPerlin(map, FREQ, THRES, enemy_char, floor_char, MAXSPAWN, SECURITYRATIO); //No funciono el Perlin Noise :c
        PlaceExit(map, rooms);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "Salas generadas: " << rooms.size() << "\n";

        std::cout << "Conexiones generadas: " << conexiones << "\n";

         for (const auto& row : map)
         {
            for (char c : row)
            {
               if (c == enemy_char)
               {
                  enemigos_total++;
               }
            }
         } 

        std::cout << "Enemigos colocados: " << enemigos_total << "\n";
        PrintMap(map);
        std::cout << "\nTiempo en que tardo en generar el mapa :" << elapsed.count() << "ms\n";
        delete root;

        std::cout << "\nENTER = Nuevo mapa [Mismos parametros]  |   ESC = Salir\n";

        char c = getch();
        if (c == 27)
        {
           break;
        } 
        while (c != 10 && c != 27)
        {
           c = getch();
        } 
        if (c == 27)
        {
          break;
        } 
    }
    
    std::cout << "Quiere volver a generar un mapa? [Distintos parametros] || [0]. Si | [2]. No\n";
    std::cin >> opcion;

    if(opcion == 2)
    {
      ciclo = false;
      std::cout << "Terminando proceso...\n";
    }
   
  } while (ciclo);
    
   
    return 0;
}

