#include <GL/freeglut.h>
#include <GL/glu.h>

#include <vector>

using namespace std;

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// Размеры окна.
GLint Width = 800;
GLint Height = 600;

// Шаг перемещения текущего набора.
const GLint MOVE_STEP = 10;

// Размеры отображаемых вершин.
const GLfloat POINT_SIZE = 4.0f;
const GLfloat ACTIVE_POINT_SIZE = 9.0f;


// Вершина примитива.
struct Point
{
    GLint x;
    GLint y;

    Point(GLint _x, GLint _y)
    {
        x = _x;
        y = _y;
    }
};


// Один графический примитив.
struct Primitive
{
    vector<Point> vertices;
};


// Набор примитивов.
struct PrimitiveSet
{
    vector<Primitive> primitives;

    GLubyte colorR = 40;
    GLubyte colorG = 160;
    GLubyte colorB = 255;
};


// Все созданные наборы.
vector<PrimitiveSet> Sets;

// Индекс активного набора.
size_t ActiveSetIndex = 0;


// Команды контекстного меню.
enum MenuCommand
{
    MENU_COLOR_RED = 1,
    MENU_COLOR_GREEN,
    MENU_COLOR_BLUE,

    MENU_MOVE_UP,
    MENU_MOVE_DOWN,
    MENU_MOVE_LEFT,
    MENU_MOVE_RIGHT,

    MENU_PREVIOUS_SET,
    MENU_NEXT_SET,
    MENU_NEW_SET,
    MENU_DELETE_PRIMITIVE,
    MENU_DELETE_SET
};


// Создаёт пустой набор при необходимости
void EnsureCurrentSet()
{
    if (Sets.empty())
    {
        Sets.push_back(PrimitiveSet());
        ActiveSetIndex = 0;
    }
    else if (ActiveSetIndex >= Sets.size())
    {
        ActiveSetIndex = Sets.size() - 1;
    }
}


// Возвращает активный набор
PrimitiveSet& GetCurrentSet()
{
    EnsureCurrentSet();
    return Sets[ActiveSetIndex];
}


// Проверяет наличие вершин в наборе
bool HasVertices(const PrimitiveSet& set)
{
    for (size_t i = 0; i < set.primitives.size(); i++)
    {
        if (!set.primitives[i].vertices.empty())
        {
            return true;
        }
    }

    return false;
}


// Добавляет вершину в текущий примитив
void AddVertex(GLint x, GLint y)
{
    // проверка на пустой наборр
    EnsureCurrentSet();

    // получаем активный набор
    PrimitiveSet& currentSet = GetCurrentSet();

    // если в наборе нет примитивов или есть три вершины, то создаём новый треугольник
    if (currentSet.primitives.empty() ||
        currentSet.primitives.back().vertices.size() == 3)
    {
        currentSet.primitives.push_back(Primitive());
    }

    // Добавляет вершину в последний примитив
    currentSet.primitives.back().vertices.push_back(Point(x, y));
}


// Удаляет незавершённый примитив
void RemoveIncompletePrimitive()
{
    if (Sets.empty())
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();

    if (currentSet.primitives.empty())
    {
        return;
    }

    size_t vertexCount = currentSet.primitives.back().vertices.size();

    if (vertexCount > 0 && vertexCount < 3)
    {
        currentSet.primitives.pop_back();
    }
}


// Начинает новый набор примитивов
void StartNewSet()
{
    EnsureCurrentSet();

    // Незавершённый треугольник не сохраняется
    RemoveIncompletePrimitive();

    // Пустой набор не завершается
    if (!HasVertices(GetCurrentSet()))
    {
        return;
    }

    Sets.push_back(PrimitiveSet());
    ActiveSetIndex = Sets.size() - 1;
}


// Изменяет цвет текущего набора
void SetCurrentColor(GLubyte r, GLubyte g, GLubyte b)
{
    EnsureCurrentSet();

    PrimitiveSet& currentSet = GetCurrentSet();

    currentSet.colorR = r;
    currentSet.colorG = g;
    currentSet.colorB = b;
}


// Перемещает текущий набор
void MoveCurrentSet(GLint dx, GLint dy)
{
    EnsureCurrentSet();

    PrimitiveSet& currentSet = GetCurrentSet();

    // Обходит примитивы
    for (size_t i = 0; i < currentSet.primitives.size(); i++)
    {
        Primitive& primitive = currentSet.primitives[i];

        // Обходит вершины примитива
        for (size_t j = 0; j < primitive.vertices.size(); j++)
        {
            primitive.vertices[j].x += dx;
            primitive.vertices[j].y += dy;
        }
    }
}


// Удаляет последний примитив
void DeleteLastPrimitive()
{
    if (Sets.empty())
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();

    if (!currentSet.primitives.empty())
    {
        currentSet.primitives.pop_back();
    }
}


// Удаляет последний набор
void DeleteLastSet()
{
    if (!Sets.empty())
    {
        Sets.pop_back();
    }

    // Оставляет пустой активный набор
    EnsureCurrentSet();
}


// Выбираем предыдущий набор
void SelectPreviousSet()
{
    EnsureCurrentSet();

    if (ActiveSetIndex == 0)
    {
        ActiveSetIndex = Sets.size() - 1;
    }
    else
    {
        ActiveSetIndex--;
    }
}


// Выбирает следующий набор
void SelectNextSet()
{
    EnsureCurrentSet();
    ActiveSetIndex = (ActiveSetIndex + 1) % Sets.size();
}


// Отрисовывает содержимое окна
void Display()
{
    // Очищает буфер кадра
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Включает сглаживание точек
    glEnable(GL_POINT_SMOOTH);

    // Обходит все наборы
    for (size_t i = 0; i < Sets.size(); i++)
    {
        PrimitiveSet& set = Sets[i];

        // Устанавливает цвет набора
        glColor3ub(set.colorR, set.colorG, set.colorB);

        // Отрисовывает готовые треугольники
        glBegin(GL_TRIANGLES);

        // проходим по каждому трегольнику набора
        for (size_t j = 0; j < set.primitives.size(); j++)
        {
            Primitive& primitive = set.primitives[j];

            // если вершин меньше 3 - пропускаем
            if (primitive.vertices.size() != 3)
            {
                continue;
            }

            for (size_t k = 0; k < primitive.vertices.size(); k++)
            {
                glVertex2i(
                    primitive.vertices[k].x,
                    primitive.vertices[k].y
                );
            }
        }

        glEnd();

        // Выделяем активный набор
        if (i == ActiveSetIndex)
        {
            glPointSize(ACTIVE_POINT_SIZE);
        }
        else
        {
            glPointSize(POINT_SIZE);
        }

        // Отрисовываем вершины набора
        glBegin(GL_POINTS);

        for (size_t j = 0; j < set.primitives.size(); j++)
        {
            Primitive& primitive = set.primitives[j];

            for (size_t k = 0; k < primitive.vertices.size(); k++)
            {
                glVertex2i(
                    primitive.vertices[k].x,
                    primitive.vertices[k].y
                );
            }
        }

        glEnd();
    }

	// Блокируем выполнение программы пока все команды не будут выполнены
    glFlush();
}


// Обрабатываем изменение размеров окна
void Reshape(GLint w, GLint h)
{
    Width = w;
    Height = h;

    // Устанавливает область вывода
    glViewport(0, 0, w, h);

    // Настраивает систему координат
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    // Возвращает матрицу модели
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


// Обрабатываем нажатия клавиатуры
void Keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        // Цвет текущего набора
    case 'r':
    case 'R':
    case '1':
        SetCurrentColor(255, 0, 0);
        break;

    case 'g':
    case 'G':
    case '2':
        SetCurrentColor(0, 255, 0);
        break;

    case 'b':
    case 'B':
    case '3':
        SetCurrentColor(0, 0, 255);
        break;

        // Перемещение текущего набора.
    case 'w':
    case 'W':
        MoveCurrentSet(0, MOVE_STEP);
        break;

    case 's':
    case 'S':
        MoveCurrentSet(0, -MOVE_STEP);
        break;

    case 'a':
    case 'A':
        MoveCurrentSet(-MOVE_STEP, 0);
        break;

    case 'd':
    case 'D':
        MoveCurrentSet(MOVE_STEP, 0);
        break;

        // Создание нового набора.
    case ' ':
        StartNewSet();
        break;

        // Удаление последнего примитива.
    case '\b':
        DeleteLastPrimitive();
        break;

        // Удаление последнего набора.
    case 'x':
    case 'X':
        DeleteLastSet();
        break;
    }

    // Запрашивает перерисовку окна
    glutPostRedisplay();
}


// Обработчик специальных клавиш клавиатуры.
void SpecialKeyboard(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_UP:
        MoveCurrentSet(0, MOVE_STEP);
        break;

    case GLUT_KEY_DOWN:
        MoveCurrentSet(0, -MOVE_STEP);
        break;

    case GLUT_KEY_LEFT:
        MoveCurrentSet(-MOVE_STEP, 0);
        break;

    case GLUT_KEY_RIGHT:
        MoveCurrentSet(MOVE_STEP, 0);
        break;

    case GLUT_KEY_PAGE_UP:
        SelectPreviousSet();
        break;

    case GLUT_KEY_PAGE_DOWN:
        SelectNextSet();
        break;

    case GLUT_KEY_DELETE:
        DeleteLastSet();
        break;
    }

    // Запрашиваем перерисовку окна
    glutPostRedisplay();
}


// Обрабатывает нажатия мыши.
void Mouse(int button, int state, int x, int y)
{
    // Обрабатывает только нажатие.
    if (state != GLUT_DOWN)
    {
        return;
    }

    // Добавляет вершину по левому клику.
    if (button == GLUT_LEFT_BUTTON)
    {
        AddVertex(x, Height - y);

        // Запрашивает перерисовку окна
        glutPostRedisplay();
    }
}


// Обрабатывает команды меню.
void Menu(int command)
{
    switch (command)
    {
    case MENU_COLOR_RED:
        SetCurrentColor(255, 0, 0);
        break;

    case MENU_COLOR_GREEN:
        SetCurrentColor(0, 255, 0);
        break;

    case MENU_COLOR_BLUE:
        SetCurrentColor(0, 0, 255);
        break;

    case MENU_MOVE_UP:
        MoveCurrentSet(0, MOVE_STEP);
        break;

    case MENU_MOVE_DOWN:
        MoveCurrentSet(0, -MOVE_STEP);
        break;

    case MENU_MOVE_LEFT:
        MoveCurrentSet(-MOVE_STEP, 0);
        break;

    case MENU_MOVE_RIGHT:
        MoveCurrentSet(MOVE_STEP, 0);
        break;

    case MENU_PREVIOUS_SET:
        SelectPreviousSet();
        break;

    case MENU_NEXT_SET:
        SelectNextSet();
        break;

    case MENU_NEW_SET:
        StartNewSet();
        break;

    case MENU_DELETE_PRIMITIVE:
        DeleteLastPrimitive();
        break;

    case MENU_DELETE_SET:
        DeleteLastSet();
        break;
    }

    // Запрашивает перерисовку окна
    glutPostRedisplay();
}


// Создаёт контекстное меню.
void CreateContextMenu()
{
    // Подменю выбора цвета
    int colorMenu = glutCreateMenu(Menu);

    glutAddMenuEntry("Red (R / 1)", MENU_COLOR_RED);
    glutAddMenuEntry("Green (G / 2)", MENU_COLOR_GREEN);
    glutAddMenuEntry("Blue (B / 3)", MENU_COLOR_BLUE);

    // Подменю перемещения
    int moveMenu = glutCreateMenu(Menu);

    glutAddMenuEntry("Up (Arrow Up / W)", MENU_MOVE_UP);
    glutAddMenuEntry("Down (Arrow Down / S)", MENU_MOVE_DOWN);
    glutAddMenuEntry("Left (Arrow Left / A)", MENU_MOVE_LEFT);
    glutAddMenuEntry("Right (Arrow Right / D)", MENU_MOVE_RIGHT);

    // Основное меню
    glutCreateMenu(Menu);

    glutAddSubMenu("Color", colorMenu);
    glutAddSubMenu("Move", moveMenu);

    glutAddMenuEntry("Previous set (Page Up)", MENU_PREVIOUS_SET);
    glutAddMenuEntry("Next set (Page Down)", MENU_NEXT_SET);
    glutAddMenuEntry("New set (Space)", MENU_NEW_SET);
    glutAddMenuEntry("Delete last primitive (Backspace)", MENU_DELETE_PRIMITIVE);
    glutAddMenuEntry("Delete last set (Delete / X)", MENU_DELETE_SET);

    // открываем меню на ПКМ
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
    // Создаём первый набор
    Sets.push_back(PrimitiveSet());

    // Инициализируем GLUT
    glutInit(&argc, argv);

    // Настраивает буфер кадра.
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

    // Задаёт размер окна.
    glutInitWindowSize(Width, Height);

    // Создаёт окно.
    glutCreateWindow("Computer Graphics - Lab 1");

    // Регистрируем callback-функции

	// при перерисовке окна вызываем Display
    glutDisplayFunc(Display);

    // при изменении размера окна вызываем Reshape
    glutReshapeFunc(Reshape);

	// при нажатии мыши вызываем Mouse, при нажатии клавы, вызываем соответствующие функции
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKeyboard);
    glutMouseFunc(Mouse);

    // Создаёт контекстное меню.
    CreateContextMenu();

    // Запускает цикл обработки событий.
    glutMainLoop();

    return 0;
}
