/*
 * ============================================================
 *   PLANO CARTESIANO 2D -- k-NN & Clustering  [v3]
 * ============================================================
 *  Lenguaje : C++
 * ============================================================
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>
#include <sstream>

// ============================================================
//  CONSTANTES
// ============================================================
static const int CANVAS_W   = 63;   // debe ser impar para centrar eje Y
static const int CANVAS_H   = 29;   // debe ser impar para centrar eje X
static const int AXIS_X_MIN = -10;
static const int AXIS_X_MAX =  10;
static const int AXIS_Y_MIN = -7;
static const int AXIS_Y_MAX =  7;
static const int MAX_ITER   = 300;
static const int MENU_W     = 44;   // ancho barra de menu lateral

static const std::vector<char> GROUP_SYMBOLS = {
    'o', '#', '@', 'S', '%', '&', 'V', '?', 'Z', 'W'
};

// ============================================================
//  ESTRUCTURAS
// ============================================================
struct Point {
    std::string name;
    double x, y;
    int groupId;
    Point() : x(0), y(0), groupId(-1) {}
    Point(std::string n, double px, double py)
        : name(n), x(px), y(py), groupId(-1) {}
};

struct Group {
    std::string name;
    char symbol;
    Point centroid;
};

struct DistancePair {
    std::string pointName;
    double distance;
};

// ============================================================
//  DISTANCIA EUCLIDIANA  O(1)
// ============================================================
double euclideanDistance(const Point& a, const Point& b) {
    double dx = b.x - a.x, dy = b.y - a.y;
    return std::sqrt(dx*dx + dy*dy);
}

// ============================================================
//  MAPEADO DE COORDENADAS -> CELDA DEL CANVAS
// ============================================================
int mapX(double x) {
    double r = (x - AXIS_X_MIN) / (double)(AXIS_X_MAX - AXIS_X_MIN);
    int c = (int)std::round(r * (CANVAS_W - 1));
    return std::max(0, std::min(CANVAS_W - 1, c));
}
int mapY(double y) {
    double r = (AXIS_Y_MAX - y) / (double)(AXIS_Y_MAX - AXIS_Y_MIN);
    int row = (int)std::round(r * (CANVAS_H - 1));
    return std::max(0, std::min(CANVAS_H - 1, row));
}

// ============================================================
//  DIBUJAR PLANO CON GRID DE PUNTOS EN CADA INTERSECCION
// ============================================================
/*
 * El plano se rellena con '.' en cada celda de la grilla.
 * Los ejes X e Y sobreescriben con '-' y '|'.
 * Las intersecciones de la grilla (enteros) se marcan con '+'.
 * Los ejes sobreescriben: en el eje X aparece '-', en eje Y '|',
 * en el origen '+', y los puntos del dataset sobreescriben todo.
 *
 * Estructura del buffer: CANVAS_H filas x CANVAS_W columnas.
 * Cada celda corresponde a una coordenada real (entero o no).
 * Solo se dibujan las celdas que corresponden a coordenadas
 * enteras como puntos de grilla '.'.
 */
void drawPlane(const std::vector<Point>& points,
               const std::vector<Group>& groups,
               const std::string& title)
{
    // --- 1. Buffer vacio ---
    std::vector<std::string> cvs(CANVAS_H, std::string(CANVAS_W, ' '));

    // --- 2. Grid: '.' en cada interseccion entera ---
    for (int yi = AXIS_Y_MIN; yi <= AXIS_Y_MAX; ++yi) {
        for (int xi = AXIS_X_MIN; xi <= AXIS_X_MAX; ++xi) {
            int c = mapX((double)xi);
            int r = mapY((double)yi);
            if (r >= 0 && r < CANVAS_H && c >= 0 && c < CANVAS_W)
                cvs[r][c] = '.';
        }
    }

    // --- 3. Eje X (y=0): sobreescribe con '-' ---
    int rowX = mapY(0);
    for (int c = 0; c < CANVAS_W; ++c)
        if (cvs[rowX][c] != '.') cvs[rowX][c] = '-';
        else                     cvs[rowX][c] = '+';   // interseccion de eje con grilla
    // Los puntos ya eran '.', ahora son '+' en eje X

    // Volver a pasar: toda la fila del eje X que sea '.' -> '+'
    // y el resto que sea ' ' -> '-'
    for (int c = 0; c < CANVAS_W; ++c) {
        char ch = cvs[rowX][c];
        if (ch == '.' || ch == '+') cvs[rowX][c] = '+';
        else                         cvs[rowX][c] = '-';
    }

    // --- 4. Eje Y (x=0): sobreescribe ---
    int colY = mapX(0);
    for (int r = 0; r < CANVAS_H; ++r) {
        char ch = cvs[r][colY];
        if (ch == '.' || ch == '+' || ch == '-') cvs[r][colY] = '+';
        else                                      cvs[r][colY] = '|';
    }

    // --- 5. Etiquetas numericas en ejes ---
    // Eje X: numeros debajo del eje (si caben)
    // Los ponemos en la misma fila del eje, a la derecha del '+' de cada tick
    for (int xi = AXIS_X_MIN; xi <= AXIS_X_MAX; xi += 2) {
        if (xi == 0) continue;
        int c = mapX((double)xi);
        // El numero lo ponemos 1 fila abajo si hay espacio
        if (rowX + 1 < CANVAS_H) {
            std::string num = std::to_string(xi);
            for (int k = 0; k < (int)num.size() && c+k < CANVAS_W; ++k)
                if (cvs[rowX+1][c+k] == ' ' || cvs[rowX+1][c+k] == '.')
                    cvs[rowX+1][c+k] = num[k];
        }
    }
    // Eje Y: numeros a la derecha del eje
    for (int yi = AXIS_Y_MIN; yi <= AXIS_Y_MAX; yi += 2) {
        if (yi == 0) continue;
        int r = mapY((double)yi);
        if (colY + 1 < CANVAS_W) {
            std::string num = std::to_string(yi);
            for (int k = 0; k < (int)num.size() && colY+1+k < CANVAS_W; ++k)
                if (cvs[r][colY+1+k] == ' ' || cvs[r][colY+1+k] == '.')
                    cvs[r][colY+1+k] = num[k];
        }
    }

    // --- 6. Proyectar puntos (maxima prioridad) ---
    for (const auto& p : points) {
        int col = mapX(p.x), row = mapY(p.y);
        if (col < 0 || col >= CANVAS_W || row < 0 || row >= CANVAS_H) continue;
        char sym = 'O';
        if (p.groupId >= 0 && p.groupId < (int)groups.size())
            sym = groups[p.groupId].symbol;
        cvs[row][col] = sym;
        // Etiqueta a la derecha
        std::string label = p.name.substr(0, 4);
        for (int k = 0; k < (int)label.size(); ++k) {
            int lc = col + 1 + k;
            if (lc < CANVAS_W && (cvs[row][lc] == ' ' || cvs[row][lc] == '.'))
                cvs[row][lc] = label[k];
        }
    }

    // --- 7. Imprimir ---
    int totalW = CANVAS_W + 2;
    // Borde superior
    std::cout << "\n  +";
    for (int i = 0; i < totalW; ++i) std::cout << "-";
    std::cout << "+\n";
    // Titulo centrado
    int pad = (totalW - (int)title.size()) / 2;
    std::cout << "  |";
    for (int i = 0; i < pad; ++i) std::cout << " ";
    std::cout << title;
    for (int i = 0; i < totalW - pad - (int)title.size(); ++i) std::cout << " ";
    std::cout << "|\n  +";
    for (int i = 0; i < totalW; ++i) std::cout << "-";
    std::cout << "+\n";
    // Contenido
    for (int r = 0; r < CANVAS_H; ++r)
        std::cout << "  | " << cvs[r] << " |\n";
    // Borde inferior
    std::cout << "  +";
    for (int i = 0; i < totalW; ++i) std::cout << "-";
    std::cout << "+\n";
    // Leyenda
    if (!groups.empty()) {
        std::cout << "  Leyenda:";
        for (int i = 0; i < (int)groups.size(); ++i)
            std::cout << "  [" << groups[i].symbol << "]=" << groups[i].name;
        std::cout << "\n";
    }
    std::cout << "  Grid '.': cada entero | '+': interseccion de ejes/grilla\n\n";
}

// ============================================================
//  LISTAR PUNTOS
// ============================================================
void listPoints(const std::vector<Point>& points) {
    if (points.empty()) { std::cout << "  (sin puntos)\n"; return; }
    std::cout << "\n  +----------+----------+----------+----------+\n";
    std::cout <<   "  |  Nombre  |    X     |    Y     |  Grupo   |\n";
    std::cout <<   "  +----------+----------+----------+----------+\n";
    for (const auto& p : points) {
        std::string grp = (p.groupId >= 0)
                        ? ("G-" + std::to_string(p.groupId+1))
                        : "--";
        std::cout << "  | " << std::left  << std::setw(8) << p.name   << " | "
                  << std::right << std::setw(8) << std::fixed
                  << std::setprecision(2) << p.x << " | "
                  << std::setw(8) << p.y << " | "
                  << std::left  << std::setw(8) << grp   << " |\n";
    }
    std::cout << "  +----------+----------+----------+----------+\n";
}

// ============================================================
//  k-NN  O(n log n)
// ============================================================
std::vector<DistancePair> kNN(const Point& q,
                               const std::vector<Point>& pts, int k) {
    std::vector<DistancePair> d;
    for (const auto& p : pts) {
        if (p.name == q.name) continue;
        d.push_back({p.name, euclideanDistance(q, p)});
    }
    std::sort(d.begin(), d.end(),
        [](const DistancePair& a, const DistancePair& b){ return a.distance < b.distance; });
    if ((int)d.size() > k) d.resize(k);
    return d;
}

void printKNN(const Point& q, const std::vector<DistancePair>& nb) {
    std::cout << "\n  k-NN para: " << q.name
              << " (" << q.x << ", " << q.y << ")\n";
    std::cout << "  +-----+----------+----------------+\n"
              << "  |  #  |  Vecino  |   Distancia    |\n"
              << "  +-----+----------+----------------+\n";
    for (int i = 0; i < (int)nb.size(); ++i)
        std::cout << "  |  " << (i+1) << "  | "
                  << std::left  << std::setw(8) << nb[i].pointName << " | "
                  << std::right << std::setw(14) << std::fixed
                  << std::setprecision(6) << nb[i].distance << " |\n";
    std::cout << "  +-----+----------+----------------+\n";
    if (!nb.empty())
        std::cout << "  >> Mas cercano: " << nb[0].pointName
                  << "  (d = " << std::fixed << std::setprecision(4)
                  << nb[0].distance << ")\n";
}

// ============================================================
//  K-MEANS  O(I*k*n)
// ============================================================
std::vector<Group> kMeans(std::vector<Point>& pts, int k) {
    int n = (int)pts.size();
    if (k <= 0 || n == 0) return {};
    if (k > n) k = n;
    std::mt19937 rng(42);
    std::vector<Point> cents;
    std::uniform_int_distribution<int> pick(0, n-1);
    cents.push_back(pts[pick(rng)]);
    for (int c = 1; c < k; ++c) {
        std::vector<double> d2(n); double tot = 0;
        for (int i = 0; i < n; ++i) {
            double best = std::numeric_limits<double>::max();
            for (auto& ct : cents) { double d = euclideanDistance(pts[i],ct); best=std::min(best,d); }
            d2[i] = best*best; tot += d2[i];
        }
        std::uniform_real_distribution<double> spin(0, tot);
        double tgt = spin(rng), acc = 0; int ch = 0;
        for (int i = 0; i < n; ++i) { acc += d2[i]; if (acc >= tgt){ ch=i; break; } }
        cents.push_back(pts[ch]);
    }
    for (int it = 0; it < MAX_ITER; ++it) {
        bool changed = false;
        for (auto& p : pts) {
            int best = 0; double bD = euclideanDistance(p, cents[0]);
            for (int c = 1; c < k; ++c) { double d = euclideanDistance(p,cents[c]); if(d<bD){bD=d;best=c;} }
            if (p.groupId != best) { p.groupId = best; changed = true; }
        }
        if (!changed) { std::cout << "  K-Means convergio en iteracion " << it+1 << "\n"; break; }
        std::vector<double> sx(k,0), sy(k,0); std::vector<int> cnt(k,0);
        for (auto& p : pts) { sx[p.groupId]+=p.x; sy[p.groupId]+=p.y; cnt[p.groupId]++; }
        for (int c = 0; c < k; ++c) if (cnt[c]) { cents[c].x=sx[c]/cnt[c]; cents[c].y=sy[c]/cnt[c]; }
    }
    std::vector<Group> gs(k);
    for (int c = 0; c < k; ++c) {
        gs[c].name    = "Grupo-" + std::to_string(c+1);
        gs[c].symbol  = GROUP_SYMBOLS[c % (int)GROUP_SYMBOLS.size()];
        gs[c].centroid = cents[c];
        gs[c].centroid.name = "C" + std::to_string(c+1);
    }
    return gs;
}

void printClusterStats(const std::vector<Point>& pts, const std::vector<Group>& gs) {
    std::cout << "\n  +----------------+--------+---------------------------+\n"
              << "  |     Grupo      | Puntos |       Centroide           |\n"
              << "  +----------------+--------+---------------------------+\n";
    for (int i = 0; i < (int)gs.size(); ++i) {
        int cnt = 0; for (auto& p:pts) if(p.groupId==i) cnt++;
        std::cout << "  | " << std::left  << std::setw(14) << gs[i].name << " | "
                  << std::right << std::setw(6) << cnt << " | ("
                  << std::fixed << std::setprecision(2)
                  << std::setw(6) << gs[i].centroid.x << ", "
                  << std::setw(6) << gs[i].centroid.y << ")           |\n";
    }
    std::cout << "  +----------------+--------+---------------------------+\n\n";
}

// ============================================================
//  CLASIFICACION  O(k)
// ============================================================
int classifyPoint(const Point& q, const std::vector<Group>& gs) {
    int best = 0; double bD = euclideanDistance(q, gs[0].centroid);
    for (int i = 1; i < (int)gs.size(); ++i) {
        double d = euclideanDistance(q, gs[i].centroid);
        if (d < bD) { bD = d; best = i; }
    }
    return best;
}

// ============================================================
//  UTILIDADES
// ============================================================
int findPoint(const std::vector<Point>& pts, const std::string& nm) {
    for (int i = 0; i < (int)pts.size(); ++i) if (pts[i].name == nm) return i;
    return -1;
}

// Devuelve string con los nombres de todos los puntos
std::string pointNamesList(const std::vector<Point>& pts) {
    if (pts.empty()) return "(ninguno)";
    std::string s;
    for (int i = 0; i < (int)pts.size(); ++i) {
        if (i) s += ", ";
        s += pts[i].name;
    }
    return s;
}

// Imprime una linea separadora
void sep(char c = '-', int w = 46) {
    std::cout << "  ";
    for (int i = 0; i < w; ++i) std::cout << c;
    std::cout << "\n";
}

// Pausa opcional
void pausar() {
    std::cout << "\n  [Enter para continuar...] ";
    std::cin.ignore(10000, '\n');
}

// ============================================================
//  CABECERA INICIAL (solo se muestra una vez al arrancar)
// ============================================================
void printHeader() {
    std::cout << "\n";
    sep('=', 46);
    std::cout << "    PLANO CARTESIANO 2D  --  k-NN & Clustering\n";
    std::cout << "    Estructuras de Datos Avanzada  |  C++\n";
    sep('=', 46);
    std::cout << "\n";
}

// ============================================================
//  BARRA DE ESTADO COMPACTA  (se imprime antes de cada prompt)
// ============================================================
/*
 * En vez de redibujar el menu completo cada vez, mostramos
 * una sola linea con el estado actual y las opciones.
 * Solo se muestra el menu completo al inicio y cuando el
 * usuario escribe 'h' o '?'.
 */
void printStatus(const std::vector<Point>& pts,
                 const std::vector<Group>& gs) {
    sep('-', 46);
    std::cout << "  Puntos: " << pts.size();
    if (!gs.empty()) std::cout << "  |  Grupos: " << gs.size();
    else             std::cout << "  |  Sin clustering";
    std::cout << "\n";
    if (!pts.empty()) std::cout << "  -> " << pointNamesList(pts) << "\n";
    sep('-', 46);
    std::cout << "  [1]Agregar [2]Eliminar [3]Listar [4]Ver plano\n";
    std::cout << "  [5]Dist    [6]k-NN     [7]Cluster [8]Clasificar\n";
    std::cout << "  [9]Demo    [h]Ayuda    [0]Salir\n";
    sep('-', 46);
    std::cout << "  > ";
}

// ============================================================
//  MENU DE AYUDA COMPLETO (solo cuando el usuario lo pide)
// ============================================================
void printHelp() {
    sep('=', 46);
    std::cout << "  AYUDA -- DESCRIPCION DE OPCIONES\n";
    sep('=', 46);
    std::cout << "  1  Agregar punto        Nombre + X + Y\n";
    std::cout << "     (calcula vecino mas cercano automaticamente)\n\n";
    std::cout << "  2  Eliminar punto       Por nombre\n\n";
    std::cout << "  3  Listar puntos        Tabla con coords y grupo\n\n";
    std::cout << "  4  Ver plano            Dibuja el plano ASCII\n";
    std::cout << "     '.' = interseccion de grilla\n";
    std::cout << "     '+' = cruce con eje\n";
    std::cout << "     Letras/simbolos = puntos del dataset\n\n";
    std::cout << "  5  Distancia            Euclidiana entre 2 puntos\n\n";
    std::cout << "  6  k-NN                 k vecinos mas cercanos\n\n";
    std::cout << "  7  Clustering K-Means   Agrupar en k grupos\n\n";
    std::cout << "  8  Clasificar           Asigna nuevo punto a grupo\n";
    std::cout << "     (requiere haber hecho clustering antes)\n\n";
    std::cout << "  9  Demo automatico      15 puntos, 3 clusters\n\n";
    std::cout << "  0  Salir\n";
    sep('=', 46);
    pausar();
}

// ============================================================
//  DEMO
// ============================================================
void runDemo(std::vector<Point>& pts, std::vector<Group>& gs) {
    sep('=', 46);
    std::cout << "  DEMO -- Dataset de 15 puntos (3 clusters naturales)\n";
    sep('=', 46);
    pts.clear(); gs.clear();
    struct R { const char* n; double x, y; };
    R data[] = {
        {"A1",-7,5},{"A2",-6,4},{"A3",-8,6},{"A4",-5,5},
        {"B1",1,1}, {"B2",2,2}, {"B3",0,0}, {"B4",1,-1},{"B5",3,1},
        {"C1",6,-5},{"C2",7,-4},{"C3",5,-6},{"C4",8,-5},{"C5",6,-3},{"C6",7,-6}
    };
    for (auto& d : data) pts.push_back(Point(d.n, d.x, d.y));

    std::cout << "\n  Paso 1/4 -- Puntos cargados:\n";
    listPoints(pts);
    pausar();

    std::cout << "\n  Paso 2/4 -- Plano con los 15 puntos:\n";
    drawPlane(pts, {}, "DATASET -- 15 PUNTOS");
    pausar();

    std::cout << "\n  Paso 3/4 -- 3-NN del punto A1:\n";
    auto nn = kNN(pts[0], pts, 3);
    printKNN(pts[0], nn);
    pausar();

    std::cout << "\n  Paso 4/4 -- K-Means k=3:\n";
    gs = kMeans(pts, 3);
    printClusterStats(pts, gs);
    drawPlane(pts, gs, "K-MEANS k=3");

    Point np("NEW", 0.5, -2.0);
    int gid = classifyPoint(np, gs);
    np.groupId = gid;
    pts.push_back(np);
    std::cout << "  NEW (0.5, -2.0) clasificado -> " << gs[gid].name
              << " [" << gs[gid].symbol << "]\n";
    drawPlane(pts, gs, "CLASIFICACION: NEW");
    pausar();
}

// ============================================================
//  MAIN
// ============================================================
int main() {
    std::vector<Point> pts;
    std::vector<Group> gs;

    printHeader();
    std::cout << "  Escribe 'h' para ver la ayuda completa.\n\n";

    std::string input;
    while (true) {
        printStatus(pts, gs);
        std::getline(std::cin, input);

        // Normalizar: tomar primer caracter no espacio
        char cmd = 0;
        for (char c : input) if (c != ' ') { cmd = c; break; }

        // --------------------------------------------------------
        if (cmd == '0') {
            std::cout << "\n  Hasta pronto!\n\n";
            break;

        // --------------------------------------------------------
        } else if (cmd == 'h' || cmd == 'H' || cmd == '?') {
            printHelp();

        // --------------------------------------------------------
        } else if (cmd == '1') {
            sep();
            std::cout << "  -- AGREGAR PUNTO --\n";
            std::string name; double x, y;
            std::cout << "  Nombre : "; std::getline(std::cin, name);
            if (name.empty()) { std::cout << "  Cancelado.\n"; continue; }
            // Limpiar espacios
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t")+1);
            if (findPoint(pts, name) >= 0) {
                std::cout << "  [!] Ya existe '" << name << "'.\n";
                pausar(); continue;
            }
            std::cout << "  X     : ";
            std::string sx; std::getline(std::cin, sx);
            std::cout << "  Y     : ";
            std::string sy; std::getline(std::cin, sy);
            try {
                x = std::stod(sx); y = std::stod(sy);
            } catch(...) {
                std::cout << "  [!] Coordenadas invalidas.\n";
                pausar(); continue;
            }
            Point np(name, x, y);
            pts.push_back(np);
            std::cout << "  [OK] Punto '" << name << "' en (" << x << ", " << y << ") agregado.\n";
            // k-NN automatico
            if (pts.size() > 1) {
                auto nn = kNN(np, pts, 1);
                std::cout << "  Vecino mas cercano: " << nn[0].pointName
                          << "  (d = " << std::fixed << std::setprecision(4)
                          << nn[0].distance << ")\n";
            }
            // Invalidar clustering previo
            gs.clear();
            for (auto& p : pts) p.groupId = -1;
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '2') {
            if (pts.empty()) { std::cout << "  [!] No hay puntos.\n"; pausar(); continue; }
            sep();
            std::cout << "  -- ELIMINAR PUNTO --\n";
            std::cout << "  Puntos: " << pointNamesList(pts) << "\n";
            std::cout << "  Nombre: ";
            std::string name; std::getline(std::cin, name);
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t")+1);
            int idx = findPoint(pts, name);
            if (idx < 0) { std::cout << "  [!] No encontrado.\n"; pausar(); continue; }
            pts.erase(pts.begin() + idx);
            gs.clear();
            for (auto& p : pts) p.groupId = -1;
            std::cout << "  [OK] '" << name << "' eliminado.\n";
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '3') {
            sep();
            std::cout << "  -- LISTA DE PUNTOS --\n";
            listPoints(pts);
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '4') {
            drawPlane(pts, gs, "PLANO CARTESIANO 2D");
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '5') {
            if (pts.size() < 2) { std::cout << "  [!] Necesitas al menos 2 puntos.\n"; pausar(); continue; }
            sep();
            std::cout << "  -- DISTANCIA EUCLIDIANA --\n";
            std::cout << "  Puntos: " << pointNamesList(pts) << "\n";
            std::cout << "  Punto A: ";
            std::string n1; std::getline(std::cin, n1);
            n1.erase(0,n1.find_first_not_of(" \t")); n1.erase(n1.find_last_not_of(" \t")+1);
            std::cout << "  Punto B: ";
            std::string n2; std::getline(std::cin, n2);
            n2.erase(0,n2.find_first_not_of(" \t")); n2.erase(n2.find_last_not_of(" \t")+1);
            int i1 = findPoint(pts, n1), i2 = findPoint(pts, n2);
            if (i1 < 0 || i2 < 0) { std::cout << "  [!] Punto(s) no encontrado(s).\n"; pausar(); continue; }
            double d = euclideanDistance(pts[i1], pts[i2]);
            std::cout << "\n  d(" << n1 << ", " << n2 << ")  =  "
                      << std::fixed << std::setprecision(6) << d << "\n";
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '6') {
            if (pts.size() < 2) { std::cout << "  [!] Necesitas al menos 2 puntos.\n"; pausar(); continue; }
            sep();
            std::cout << "  -- k-NN (VECINOS MAS CERCANOS) --\n";
            std::cout << "  Puntos: " << pointNamesList(pts) << "\n";
            std::cout << "  Punto query: ";
            std::string name; std::getline(std::cin, name);
            name.erase(0,name.find_first_not_of(" \t")); name.erase(name.find_last_not_of(" \t")+1);
            int idx = findPoint(pts, name);
            if (idx < 0) { std::cout << "  [!] No encontrado.\n"; pausar(); continue; }
            std::cout << "  k (numero de vecinos): ";
            std::string sk; std::getline(std::cin, sk);
            int k = 1;
            try { k = std::stoi(sk); } catch(...) { k = 1; }
            if (k < 1) k = 1;
            int maxK = (int)pts.size() - 1;
            if (k > maxK) k = maxK;
            auto nn = kNN(pts[idx], pts, k);
            printKNN(pts[idx], nn);
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '7') {
            if (pts.empty()) { std::cout << "  [!] Sin puntos.\n"; pausar(); continue; }
            sep();
            std::cout << "  -- CLUSTERING K-MEANS --\n";
            int maxK = std::min((int)pts.size(), (int)GROUP_SYMBOLS.size());
            std::cout << "  Numero de grupos k (1 - " << maxK << "): ";
            std::string sk; std::getline(std::cin, sk);
            int k = 2;
            try { k = std::stoi(sk); } catch(...) {}
            if (k < 1 || k > maxK) {
                std::cout << "  [!] k debe estar entre 1 y " << maxK << ".\n";
                pausar(); continue;
            }
            for (auto& p : pts) p.groupId = -1;
            gs = kMeans(pts, k);
            printClusterStats(pts, gs);
            drawPlane(pts, gs, "K-MEANS CLUSTERING");
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '8') {
            if (gs.empty()) {
                std::cout << "  [!] Ejecuta primero el clustering (opcion 7).\n";
                pausar(); continue;
            }
            sep();
            std::cout << "  -- CLASIFICAR NUEVO PUNTO --\n";
            std::cout << "  Grupos disponibles:\n";
            for (int i = 0; i < (int)gs.size(); ++i)
                std::cout << "    [" << gs[i].symbol << "] " << gs[i].name
                          << "  centroide=(" << std::fixed << std::setprecision(2)
                          << gs[i].centroid.x << ", " << gs[i].centroid.y << ")\n";
            std::cout << "  Nombre del nuevo punto: ";
            std::string name; std::getline(std::cin, name);
            name.erase(0,name.find_first_not_of(" \t")); name.erase(name.find_last_not_of(" \t")+1);
            std::cout << "  X: ";
            std::string sx; std::getline(std::cin, sx);
            std::cout << "  Y: ";
            std::string sy; std::getline(std::cin, sy);
            double x = 0, y = 0;
            try { x = std::stod(sx); y = std::stod(sy); } catch(...) {
                std::cout << "  [!] Coordenadas invalidas.\n"; pausar(); continue;
            }
            Point np(name, x, y);
            int gid = classifyPoint(np, gs);
            np.groupId = gid;
            pts.push_back(np);
            std::cout << "\n  >> '" << name << "' clasificado en: "
                      << gs[gid].name << "  [" << gs[gid].symbol << "]\n";
            drawPlane(pts, gs, "CLASIFICACION: " + name);
            pausar();

        // --------------------------------------------------------
        } else if (cmd == '9') {
            runDemo(pts, gs);

        // --------------------------------------------------------
        } else if (cmd != 0) {
            std::cout << "  [?] Comando desconocido. Escribe 'h' para ayuda.\n";
        }
    }
    return 0;
}

/*
 * ============================================================
 *  COMPLEJIDAD
 * ============================================================
 *  euclideanDistance : O(1)
 *  k-NN              : O(n log n)
 *  K-Means (Lloyd)   : O(I * k * n),  I <= 300
 *  K-Means++ init    : O(k * n)
 *  Clasificacion     : O(k)
 *  drawPlane         : O(W * H)
 * ============================================================
 */
