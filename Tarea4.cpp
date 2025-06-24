#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>

//Settings del mapa

const int WIDTH = 40;
const int HEIGHT = 20;
const int MIN_ROOM_SIZE = 4;
const int MIN_LEAF_SIZE = 8;

struct Room 
{
  int x, y, w, h;
  int center_x() const {return x + w /2; }
  int center_y() const {return y + h /2;}
};

struct Leaf
{
  int x, y, w, h;
  Leaf* left = nullptr;
  Leaf* right = nullptr;
  Room* room = nullptr;

  Leaf(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    ~Leaf() { delete left; delete right; delete room; }
};

//=======================RNG global======================//
std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));

//==================Divide la "hoja" en dos partes que son hijas===============//
bool Split(Leaf* leaf)
{

   if(leaf ->left || leaf ->right)
   {
     return false;
   }
  
  bool splitH = (leaf->w > leaf->h);
  if(leaf->w / leaf->h >= 1.25)
  {
    splitH = false;
  }
  else if(leaf->h / leaf->w >= 1.25)
  {
   splitH = true;
  }
   
  std::uniform_int_distribution<int> split_dist(MIN_LEAF_SIZE, (splitH ? leaf->h : leaf->w) - MIN_LEAF_SIZE);

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

    for(auto l : new_leaves) leaves.push_back(l);
  }while(did_split);
}

//==============GENERADOR DE ROOM CON LAS HOJAS DEL ARBOL============//
void CreateRoom(Leaf* leaf)
{
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
  std::uniform_int_distribution<int> room_w_dist(MIN_ROOM_SIZE, leaf->w - 2);
  std::uniform_int_distribution<int> room_h_dist(MIN_ROOM_SIZE, leaf->h - 2);
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
 if (leaf->left || leaf->right)
{
    if (leaf->left) FillRoom(map, leaf->left, rooms);
    if (leaf->right) FillRoom(map, leaf->right, rooms);
}
else if (leaf->room)
{
    rooms.push_back(leaf->room);
    for (int y = leaf->room->y; y < leaf->room->y + leaf->room->h; ++y)
        for (int x = leaf->room->x; x < leaf->room->x + leaf->room->w; ++x)
            map[y][x] = '.';
}
}

//====================CONECTA LOS CENTROS DE LAS ROOM HACIENDO UN PASILLO=========//
void ConnectRooms(std::vector<std::vector<char>>& map, Room* a, Room* b)
{
 int x1 = a->center_x(), y1 = a->center_y();
 int x2 = b->center_x(), y2 = b->center_y();

 if(rng() %2)
 {
   for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) map[y1][x] = '.';
   for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) map[y][x2] = '.';
 }
 else
 {
  for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) map[y][x1] = '.';
  for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) map[y2][x] = '.';
 }
}

//===============CONECTA LAS ROOMS EN EL ARBOL BSP========//
void ConnectLeafRoom(std::vector<std::vector<char>>& map, Leaf* leaf)
{
 if(leaf->left && leaf->right)
 {
  Room* a = nullptr;
  Room* b = nullptr;
  // Busca una habitación en cada subárbol
  std::vector<Leaf*> stack;
  stack.push_back(leaf->left);
  while (!stack.empty()) 
  {
     Leaf* l = stack.back(); stack.pop_back();
     if (l->room) { a = l->room; break; }
     if (l->left) stack.push_back(l->left);
     if (l->right) stack.push_back(l->right);
  }

  stack.push_back(leaf->right);
  while(!stack.empty())
  {
    Leaf* l = stack.back(); stack.pop_back();
    if (l->room) { b = l->room; break; }
    if (l->left) stack.push_back(l->left);
    if (l->right) stack.push_back(l->right);
  }
   if(a && b) ConnectRooms(map, a, b); 
   ConnectLeafRoom(map, leaf->left);
   ConnectLeafRoom(map, leaf->right);
 }
}

//===============SPAWn DEL PLAYER EN LA ROOM 1==========//
void PlacePlayer(std::vector<std::vector<char>>& map, std::vector<Room*>& rooms) 
{
    if (!rooms.empty()) 
    {
        map[rooms[0]->center_y()][rooms[0]->center_x()] = 'P';
    }
}

//================GENERAR ENEMIGOS==============//
void PlacedEnemy(std::vector<std::vector<char>>& map, std::vector<Room*>& rooms, int enemigos_por_habitacion = 1)
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

        if(map[ey][ex] == '.')
        {
          map[ey][ex] = 'E';
          enemigos_colocados++;
        }
        intentos++;
     }
  }
}

//==============MOSTRAR EL MAPA==========//
void PrintMap(const std::vector<std::vector<char>>& map)
{
for (const auto& row : map)
{
    for (char c : row)
        std::cout << c;
    std::cout << "\n";
}
}

int main()
{
    std::vector<std::vector<char>> map(HEIGHT, std::vector<char>(WIDTH, '#'));
    Leaf* root = new Leaf(0, 0, WIDTH, HEIGHT);

    std::vector<Leaf*> leaves = { root };
    SplitAllLeaves(leaves);
    CreateRoom(root);

    std::vector<Room*> rooms;
    FillRoom(map, root, rooms);

    ConnectLeafRoom(map, root);
    PlacePlayer(map, rooms);
    PlacedEnemy(map, rooms, 5);
    PrintMap(map);

    delete root;


  return 0;
}

