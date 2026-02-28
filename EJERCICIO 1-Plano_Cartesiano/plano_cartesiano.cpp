#include <iostream>
#include <vector>

using namespace std;

// Dimensiones del plano cartesiano
const int MAX_X = 20;
const int MAX_Y = 20;

// ================== CLASE PLANO ==================
class PlanoCartesiano {
private:
    char plano[MAX_Y][MAX_X];

public:
    PlanoCartesiano() {
        reiniciarPlano();
    }

    void reiniciarPlano() {
        for (int i = 0; i < MAX_Y; i++) {
            for (int j = 0; j < MAX_X; j++) {
                plano[i][j] = '.';
            }
        }
    }

    void colocarPunto(int x, int y, char simbolo) {
        if (x < 0 || x >= MAX_X || y < 0 || y >= MAX_Y) {
            cout << "Error: Coordenada fuera del plano.\n";
            return;
        }

        int fila = (MAX_Y - 1) - y;
        int columna = x;

        plano[fila][columna] = simbolo;
    }

    void imprimirPlano() {
        cout << "\n      PLANO CARTESIANO (EJE Y)\n\n";

        for (int i = 0; i < MAX_Y; i++) {
            int valorY = MAX_Y - 1 - i;
            if (valorY < 10) cout << " " << valorY << " | ";
            else             cout << valorY << " | ";

            for (int j = 0; j < MAX_X; j++) {
                cout << plano[i][j] << " ";
            }
            cout << endl;
        }

        cout << "    ";
        for (int i = 0; i < MAX_X; i++) cout << "--";
        cout << endl;

        cout << "      ";
        for (int i = 0; i < MAX_X; i++) {
            if (i < 10) cout << i << " ";
            else        cout << i;
        }
        cout << "\n        EJE X\n";
    }
};

// ================== CLASE PUNTO ==================
class Punto2D {
private:
    int x;
    int y;

public:
    Punto2D(int coordX, int coordY) {
        x = coordX;
        y = coordY;
    }

    void dibujar(PlanoCartesiano &plano) {
        plano.colocarPunto(x, y, 'O');
    }
};

// ================== MAIN ==================
int main() {
    PlanoCartesiano plano;
    vector<Punto2D> puntos;

    int opcion;
    int x, y;

    do {
        plano.reiniciarPlano();

        for (Punto2D &p : puntos) {
            p.dibujar(plano);
        }

        plano.imprimirPlano();

        cout << "\nMENU\n";
        cout << "1. Agregar punto\n";
        cout << "2. Eliminar todos los puntos\n";
        cout << "3. Salir\n";
        cout << "Opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1:
                cout << "Ingrese X (0-" << MAX_X - 1 << "): ";
                cin >> x;
                cout << "Ingrese Y (0-" << MAX_Y - 1 << "): ";
                cin >> y;
                puntos.push_back(Punto2D(x, y));
                break;

            case 2:
                puntos.clear();
                cout << "Todos los puntos fueron eliminados.\n";
                break;

            case 3:
                cout << "Finalizando programa...\n";
                break;

            default:
                cout << "Opcion invalida.\n";
        }

    } while (opcion != 3);

    return 0;
}
