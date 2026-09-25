#include <GL/freeglut.h>
#include <GL/glu.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace std;

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// Размеры окна
GLint Width = 800;
GLint Height = 600;

// Шаг перемещения текущего набора
const GLint MOVE_STEP = 10;

// Размеры отображаемых вершин
const GLfloat POINT_SIZE = 4.0f;
const GLfloat ACTIVE_POINT_SIZE = 9.0f;
const GLfloat SELECTED_POINT_SIZE = 13.0f;


// Вершина примитива
struct Point
{
    GLint x;
    GLint y;
    GLint initialX;
    GLint initialY;

    Point(GLint _x, GLint _y)
    {
        x = _x;
        y = _y;
        initialX = _x;
        initialY = _y;
    }
};


// Один графический примитив
struct Primitive
{
    vector<Point> vertices;

    GLubyte colorR;
    GLubyte colorG;
    GLubyte colorB;
    GLint zOrder;

    Primitive(
        GLubyte _colorR = 40,
        GLubyte _colorG = 160,
        GLubyte _colorB = 255,
        GLint _zOrder = 0
    )
    {
        colorR = _colorR;
        colorG = _colorG;
        colorB = _colorB;
        zOrder = _zOrder;
    }
};


// Набор примитивов
struct PrimitiveSet
{
    vector<Primitive> primitives;

    GLubyte colorR = 40;
    GLubyte colorG = 160;
    GLubyte colorB = 255;
    GLint zOrder = 0;
};


// Все созданные наборы
vector<PrimitiveSet> Sets;

// Индекс активного набора
size_t ActiveSetIndex = 0;

// Режим взаимодействия с программой
enum InteractionMode
{
    DRAW_MODE,
    EDIT_MODE
};

InteractionMode CurrentMode = DRAW_MODE;

// Выбранный примитив активного набора
size_t ActivePrimitiveIndex = 0;
bool HasActivePrimitive = false;

// Контекстные меню режимов
int DrawMenuId = 0;
int EditMenuId = 0;

void AttachContextMenu();


// Команды контекстного меню
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
    MENU_LAYER_FORWARD,
    MENU_LAYER_BACKWARD,
    MENU_LAYER_FRONT,
    MENU_LAYER_BACK,
    MENU_TOGGLE_MODE,
    MENU_PREVIOUS_PRIMITIVE,
    MENU_NEXT_PRIMITIVE,
    MENU_RESET_POSITION,
    MENU_NEW_SET,
    MENU_DELETE_PRIMITIVE,
    MENU_DELETE_ACTIVE_SET,
    MENU_DELETE_SET
};


// Создаёт пустой набор при необходимости и корректирует индекс
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


// Проверяет, завершён ли треугольник
bool IsCompletedPrimitive(const Primitive& primitive)
{
    return primitive.vertices.size() == 3;
}


// Возвращает количество завершённых примитивов набора
size_t GetCompletedPrimitiveCount(const PrimitiveSet& set)
{
    size_t count = 0;

    for (size_t i = 0; i < set.primitives.size(); i++)
    {
        if (IsCompletedPrimitive(set.primitives[i]))
        {
            count++;
        }
    }

    return count;
}


// Сбрасывает выбор примитива
void ClearActivePrimitive()
{
    ActivePrimitiveIndex = 0;
    HasActivePrimitive = false;
}


// Проверяет индекс выбранного примитива
void EnsureActivePrimitive()
{
    PrimitiveSet& currentSet = GetCurrentSet();
    size_t primitiveCount = GetCompletedPrimitiveCount(currentSet);

    if (primitiveCount == 0)
    {
        ClearActivePrimitive();
    }
    else if (!HasActivePrimitive || ActivePrimitiveIndex >= primitiveCount)
    {
        ActivePrimitiveIndex = primitiveCount - 1;
        HasActivePrimitive = true;
    }
}


// Выбирает предыдущий примитив активного набора
void SelectPreviousPrimitive()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    if (ActivePrimitiveIndex == 0)
    {
        ActivePrimitiveIndex = GetCompletedPrimitiveCount(GetCurrentSet()) - 1;
    }
    else
    {
        ActivePrimitiveIndex--;
    }
}


// Выбирает следующий примитив активного набора
void SelectNextPrimitive()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    size_t primitiveCount = GetCompletedPrimitiveCount(GetCurrentSet());
    ActivePrimitiveIndex = (ActivePrimitiveIndex + 1) % primitiveCount;
}


// Переключает режим рисования и редактирования
void ToggleInteractionMode()
{
    if (CurrentMode == DRAW_MODE)
    {
        CurrentMode = EDIT_MODE;
        ClearActivePrimitive();
        EnsureActivePrimitive();
    }
    else
    {
        CurrentMode = DRAW_MODE;
        ClearActivePrimitive();
    }

    AttachContextMenu();
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


// Возвращает порядок отрисовки наборов
vector<size_t> GetSetDrawOrder()
{
    vector<size_t> order;

    for (size_t i = 0; i < Sets.size(); i++)
    {
        order.push_back(i);
    }

    sort(order.begin(), order.end(), [](size_t first, size_t second)
    {
        if (Sets[first].zOrder == Sets[second].zOrder)
        {
            return first < second;
        }

        return Sets[first].zOrder < Sets[second].zOrder;
    });

    return order;
}


// Возвращает положение активного набора среди слоёв
size_t GetActiveLayerPosition()
{
    vector<size_t> order = GetSetDrawOrder();

    for (size_t i = 0; i < order.size(); i++)
    {
        if (order[i] == ActiveSetIndex)
        {
            return i;
        }
    }

    return 0;
}


// Возвращает верхнее значение порядка слоя
GLint GetHighestZOrder()
{
    GLint highest = 0;

    for (size_t i = 0; i < Sets.size(); i++)
    {
        if (Sets[i].zOrder > highest)
        {
            highest = Sets[i].zOrder;
        }
    }

    return highest;
}


// Поднимает активный набор на один слой
void MoveCurrentSetForward()
{
    if (CurrentMode != DRAW_MODE)
    {
        return;
    }

    vector<size_t> order = GetSetDrawOrder();
    size_t position = GetActiveLayerPosition();

    if (position + 1 < order.size())
    {
        swap(Sets[ActiveSetIndex].zOrder, Sets[order[position + 1]].zOrder);
    }
}


// Опускает активный набор на один слой
void MoveCurrentSetBackward()
{
    if (CurrentMode != DRAW_MODE)
    {
        return;
    }

    vector<size_t> order = GetSetDrawOrder();
    size_t position = GetActiveLayerPosition();

    if (position > 0)
    {
        swap(Sets[ActiveSetIndex].zOrder, Sets[order[position - 1]].zOrder);
    }
}


// Поднимает активный набор поверх остальных
void BringCurrentSetToFront()
{
    if (CurrentMode != DRAW_MODE)
    {
        return;
    }

    vector<size_t> order = GetSetDrawOrder();
    size_t position = GetActiveLayerPosition();

    for (size_t i = position; i + 1 < order.size(); i++)
    {
        swap(Sets[ActiveSetIndex].zOrder, Sets[order[i + 1]].zOrder);
    }
}


// Опускает активный набор под остальные
void SendCurrentSetToBack()
{
    if (CurrentMode != DRAW_MODE)
    {
        return;
    }

    vector<size_t> order = GetSetDrawOrder();
    size_t position = GetActiveLayerPosition();

    for (size_t i = position; i > 0; i--)
    {
        swap(Sets[ActiveSetIndex].zOrder, Sets[order[i - 1]].zOrder);
    }
}


// Возвращает порядок отрисовки завершённых примитивов
vector<size_t> GetPrimitiveDrawOrder(const PrimitiveSet& set)
{
    vector<size_t> order;

    for (size_t i = 0; i < set.primitives.size(); i++)
    {
        if (IsCompletedPrimitive(set.primitives[i]))
        {
            order.push_back(i);
        }
    }

    sort(order.begin(), order.end(), [&set](size_t first, size_t second)
    {
        if (set.primitives[first].zOrder == set.primitives[second].zOrder)
        {
            return first < second;
        }

        return set.primitives[first].zOrder < set.primitives[second].zOrder;
    });

    return order;
}


// Возвращает положение выбранного примитива в наборе
size_t GetActivePrimitiveLayerPosition()
{
    PrimitiveSet& currentSet = GetCurrentSet();
    vector<size_t> order = GetPrimitiveDrawOrder(currentSet);

    for (size_t i = 0; i < order.size(); i++)
    {
        if (order[i] == ActivePrimitiveIndex)
        {
            return i;
        }
    }

    return 0;
}


// Возвращает верхнее значение порядка примитива
GLint GetHighestPrimitiveZOrder(const PrimitiveSet& set)
{
    GLint highest = -1;

    for (size_t i = 0; i < set.primitives.size(); i++)
    {
        if (set.primitives[i].zOrder > highest)
        {
            highest = set.primitives[i].zOrder;
        }
    }

    return highest;
}


// Поднимает выбранный примитив на один уровень
void MoveActivePrimitiveForward()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();
    vector<size_t> order = GetPrimitiveDrawOrder(currentSet);
    size_t position = GetActivePrimitiveLayerPosition();

    if (position + 1 < order.size())
    {
        swap(
            currentSet.primitives[ActivePrimitiveIndex].zOrder,
            currentSet.primitives[order[position + 1]].zOrder
        );
    }
}


// Опускает выбранный примитив на один уровень
void MoveActivePrimitiveBackward()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();
    vector<size_t> order = GetPrimitiveDrawOrder(currentSet);
    size_t position = GetActivePrimitiveLayerPosition();

    if (position > 0)
    {
        swap(
            currentSet.primitives[ActivePrimitiveIndex].zOrder,
            currentSet.primitives[order[position - 1]].zOrder
        );
    }
}


// Поднимает выбранный примитив поверх набора
void BringActivePrimitiveToFront()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();
    vector<size_t> order = GetPrimitiveDrawOrder(currentSet);
    size_t position = GetActivePrimitiveLayerPosition();

    for (size_t i = position; i + 1 < order.size(); i++)
    {
        swap(
            currentSet.primitives[ActivePrimitiveIndex].zOrder,
            currentSet.primitives[order[i + 1]].zOrder
        );
    }
}


// Опускает выбранный примитив под набор
void SendActivePrimitiveToBack()
{
    if (CurrentMode != EDIT_MODE)
    {
        return;
    }

    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();
    vector<size_t> order = GetPrimitiveDrawOrder(currentSet);
    size_t position = GetActivePrimitiveLayerPosition();

    for (size_t i = position; i > 0; i--)
    {
        swap(
            currentSet.primitives[ActivePrimitiveIndex].zOrder,
            currentSet.primitives[order[i - 1]].zOrder
        );
    }
}


// Поднимает текущий объект на один уровень
void MoveCurrentLayerForward()
{
    if (CurrentMode == EDIT_MODE)
    {
        MoveActivePrimitiveForward();
    }
    else
    {
        MoveCurrentSetForward();
    }
}


// Опускает текущий объект на один уровень
void MoveCurrentLayerBackward()
{
    if (CurrentMode == EDIT_MODE)
    {
        MoveActivePrimitiveBackward();
    }
    else
    {
        MoveCurrentSetBackward();
    }
}


// Поднимает текущий объект поверх остальных
void BringCurrentLayerToFront()
{
    if (CurrentMode == EDIT_MODE)
    {
        BringActivePrimitiveToFront();
    }
    else
    {
        BringCurrentSetToFront();
    }
}


// Опускает текущий объект под остальные
void SendCurrentLayerToBack()
{
    if (CurrentMode == EDIT_MODE)
    {
        SendActivePrimitiveToBack();
    }
    else
    {
        SendCurrentSetToBack();
    }
}


// Обновляет информацию о текущем состоянии в заголовке окна
void UpdateWindowTitle()
{
    EnsureCurrentSet();

    PrimitiveSet& currentSet = GetCurrentSet();
    size_t primitiveCount = GetCompletedPrimitiveCount(currentSet);

    string title = "Computer Graphics - Lab 1 | ";
    title += CurrentMode == DRAW_MODE ? "DRAW" : "EDIT";
    title += " | Set " + to_string(ActiveSetIndex + 1);
    title += "/" + to_string(Sets.size());
    title += " | Layer " + to_string(GetActiveLayerPosition() + 1);
    title += "/" + to_string(Sets.size());

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();

        if (HasActivePrimitive)
        {
            title += " | Primitive " + to_string(ActivePrimitiveIndex + 1);
            title += "/" + to_string(primitiveCount);
            title += " | Order " + to_string(GetActivePrimitiveLayerPosition() + 1);
            title += "/" + to_string(primitiveCount);
        }
        else
        {
            title += " | No completed primitives - F2: DRAW";
        }
    }
    else if (currentSet.primitives.empty())
    {
        title += " | EMPTY - PageUp/PageDown or delete active set from menu";
    }
    else
    {
        title += " | Triangles: " + to_string(primitiveCount);

        size_t vertexCount = currentSet.primitives.back().vertices.size();

        if (vertexCount > 0 && vertexCount < 3)
        {
            title += " | Drawing: " + to_string(vertexCount) + "/3";
        }
    }

    glutSetWindowTitle(title.c_str());
}


// Границы графического объекта
struct Bounds
{
    GLint minX = 0;
    GLint maxX = 0;
    GLint minY = 0;
    GLint maxY = 0;
    bool hasPoints = false;
};


// Добавляет точку в вычисляемые границы
void IncludePoint(Bounds& bounds, const Point& point)
{
    if (!bounds.hasPoints)
    {
        bounds.minX = point.x;
        bounds.maxX = point.x;
        bounds.minY = point.y;
        bounds.maxY = point.y;
        bounds.hasPoints = true;
        return;
    }

    if (point.x < bounds.minX) bounds.minX = point.x;
    if (point.x > bounds.maxX) bounds.maxX = point.x;
    if (point.y < bounds.minY) bounds.minY = point.y;
    if (point.y > bounds.maxY) bounds.maxY = point.y;
}


// Вычисляет границы примитива
Bounds GetPrimitiveBounds(const Primitive& primitive)
{
    Bounds bounds;

    for (size_t i = 0; i < primitive.vertices.size(); i++)
    {
        IncludePoint(bounds, primitive.vertices[i]);
    }

    return bounds;
}


// Вычисляет границы набора
Bounds GetSetBounds(const PrimitiveSet& set)
{
    Bounds bounds;

    for (size_t i = 0; i < set.primitives.size(); i++)
    {
        for (size_t j = 0; j < set.primitives[i].vertices.size(); j++)
        {
            IncludePoint(bounds, set.primitives[i].vertices[j]);
        }
    }

    return bounds;
}


// Ограничивает смещение границами рабочей области
void LimitMovement(const Bounds& bounds, GLint& dx, GLint& dy)
{
    if (!bounds.hasPoints)
    {
        dx = 0;
        dy = 0;
        return;
    }

    GLint margin = static_cast<GLint>(SELECTED_POINT_SIZE / 2.0f + 0.5f);
    GLint left = margin;
    GLint right = Width - margin;
    GLint bottom = margin;
    GLint top = Height - margin;

    if (right <= left || bounds.maxX - bounds.minX > right - left)
    {
        dx = 0;
    }
    else
    {
        if (bounds.minX + dx < left) dx = left - bounds.minX;
        if (bounds.maxX + dx > right) dx = right - bounds.maxX;
    }

    if (top <= bottom || bounds.maxY - bounds.minY > top - bottom)
    {
        dy = 0;
    }
    else
    {
        if (bounds.minY + dy < bottom) dy = bottom - bounds.minY;
        if (bounds.maxY + dy > top) dy = top - bounds.maxY;
    }
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
        currentSet.primitives.push_back(Primitive(
            currentSet.colorR,
            currentSet.colorG,
            currentSet.colorB,
            GetHighestPrimitiveZOrder(currentSet) + 1
        ));
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

    PrimitiveSet newSet;
    newSet.zOrder = GetHighestZOrder() + 1;
    Sets.push_back(newSet);
    ActiveSetIndex = Sets.size() - 1;
    CurrentMode = DRAW_MODE;
    ClearActivePrimitive();
}


// Изменяет цвет текущего объекта
void SetCurrentColor(GLubyte r, GLubyte g, GLubyte b)
{
    EnsureCurrentSet();

    PrimitiveSet& currentSet = GetCurrentSet();

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();

        if (!HasActivePrimitive)
        {
            return;
        }

        Primitive& primitive = currentSet.primitives[ActivePrimitiveIndex];
        primitive.colorR = r;
        primitive.colorG = g;
        primitive.colorB = b;
        return;
    }

    currentSet.colorR = r;
    currentSet.colorG = g;
    currentSet.colorB = b;

    for (size_t i = 0; i < currentSet.primitives.size(); i++)
    {
        currentSet.primitives[i].colorR = r;
        currentSet.primitives[i].colorG = g;
        currentSet.primitives[i].colorB = b;
    }
}


// Перемещает текущий набор
void MoveCurrentSet(GLint dx, GLint dy)
{
    EnsureCurrentSet();

    PrimitiveSet& currentSet = GetCurrentSet();
    Bounds bounds = GetSetBounds(currentSet);
    LimitMovement(bounds, dx, dy);

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


// Перемещает выбранный примитив
void MoveActivePrimitive(GLint dx, GLint dy)
{
    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    Primitive& primitive = GetCurrentSet().primitives[ActivePrimitiveIndex];
    Bounds bounds = GetPrimitiveBounds(primitive);
    LimitMovement(bounds, dx, dy);

    for (size_t i = 0; i < primitive.vertices.size(); i++)
    {
        primitive.vertices[i].x += dx;
        primitive.vertices[i].y += dy;
    }
}


// Перемещает набор или выбранный примитив в зависимости от режима
void MoveCurrentTarget(GLint dx, GLint dy)
{
    if (CurrentMode == EDIT_MODE)
    {
        MoveActivePrimitive(dx, dy);
    }
    else
    {
        MoveCurrentSet(dx, dy);
    }
}


// Возвращает весь активный набор в исходное положение
void ResetCurrentSetPosition()
{
    PrimitiveSet& currentSet = GetCurrentSet();

    for (size_t i = 0; i < currentSet.primitives.size(); i++)
    {
        Primitive& primitive = currentSet.primitives[i];

        for (size_t j = 0; j < primitive.vertices.size(); j++)
        {
            primitive.vertices[j].x = primitive.vertices[j].initialX;
            primitive.vertices[j].y = primitive.vertices[j].initialY;
        }
    }
}


// Возвращает выбранный примитив в исходное положение
void ResetActivePrimitivePosition()
{
    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    Primitive& primitive = GetCurrentSet().primitives[ActivePrimitiveIndex];

    for (size_t i = 0; i < primitive.vertices.size(); i++)
    {
        primitive.vertices[i].x = primitive.vertices[i].initialX;
        primitive.vertices[i].y = primitive.vertices[i].initialY;
    }
}


// Возвращает текущий объект в исходное положение
void ResetCurrentTargetPosition()
{
    if (CurrentMode == EDIT_MODE)
    {
        ResetActivePrimitivePosition();
    }
    else
    {
        ResetCurrentSetPosition();
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

    ClearActivePrimitive();
}


// Удаляет выбранный примитив
void DeleteActivePrimitive()
{
    EnsureActivePrimitive();

    if (!HasActivePrimitive)
    {
        return;
    }

    PrimitiveSet& currentSet = GetCurrentSet();
    currentSet.primitives.erase(
        currentSet.primitives.begin() + ActivePrimitiveIndex
    );

    size_t primitiveCount = GetCompletedPrimitiveCount(currentSet);

    if (primitiveCount == 0)
    {
        ClearActivePrimitive();
    }
    else if (ActivePrimitiveIndex >= primitiveCount)
    {
        ActivePrimitiveIndex = primitiveCount - 1;
        HasActivePrimitive = true;
    }
}


// Удаляет примитив в соответствии с текущим режимом
void DeleteCurrentPrimitive()
{
    if (CurrentMode == EDIT_MODE)
    {
        DeleteActivePrimitive();
    }
    else
    {
        DeleteLastPrimitive();
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

    ClearActivePrimitive();

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();
    }
}


// Удаляет активный набор по явной команде пользователя
void DeleteActiveSet()
{
    EnsureCurrentSet();
    Sets.erase(Sets.begin() + ActiveSetIndex);
    EnsureCurrentSet();

    ClearActivePrimitive();

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();
    }
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

    ClearActivePrimitive();

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();
    }
}


// Выбирает следующий набор
void SelectNextSet()
{
    EnsureCurrentSet();
    ActiveSetIndex = (ActiveSetIndex + 1) % Sets.size();

    ClearActivePrimitive();

    if (CurrentMode == EDIT_MODE)
    {
        EnsureActivePrimitive();
    }
}


// Отрисовывает содержимое окна
void Display()
{
    // Очищает буфер кадра
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Включает сглаживание точек
    glEnable(GL_POINT_SMOOTH);

    // Обходит наборы в порядке слоёв
    vector<size_t> drawOrder = GetSetDrawOrder();

    for (size_t orderIndex = 0; orderIndex < drawOrder.size(); orderIndex++)
    {
        size_t i = drawOrder[orderIndex];
        PrimitiveSet& set = Sets[i];
        vector<size_t> primitiveOrder = GetPrimitiveDrawOrder(set);

        // Отрисовывает готовые треугольники
        glBegin(GL_TRIANGLES);

        // Проходит по треугольникам в порядке наложения
        for (size_t orderPosition = 0;
            orderPosition < primitiveOrder.size();
            orderPosition++)
        {
            size_t j = primitiveOrder[orderPosition];
            Primitive& primitive = set.primitives[j];

            glColor3ub(primitive.colorR, primitive.colorG, primitive.colorB);

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

            glColor3ub(primitive.colorR, primitive.colorG, primitive.colorB);

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

    // Дополнительно выделяет выбранный примитив в режиме редактирования
    if (CurrentMode == EDIT_MODE &&
        ActiveSetIndex < Sets.size() &&
        HasActivePrimitive)
    {
        PrimitiveSet& currentSet = Sets[ActiveSetIndex];

        if (ActivePrimitiveIndex < currentSet.primitives.size())
        {
            Primitive& primitive = currentSet.primitives[ActivePrimitiveIndex];

            if (IsCompletedPrimitive(primitive))
            {
                glColor3ub(255, 255, 0);
                glLineWidth(3.0f);
                glBegin(GL_LINE_LOOP);

                for (size_t i = 0; i < primitive.vertices.size(); i++)
                {
                    glVertex2i(primitive.vertices[i].x, primitive.vertices[i].y);
                }

                glEnd();
                glLineWidth(1.0f);

                glPointSize(SELECTED_POINT_SIZE);
                glBegin(GL_POINTS);

                for (size_t i = 0; i < primitive.vertices.size(); i++)
                {
                    glVertex2i(primitive.vertices[i].x, primitive.vertices[i].y);
                }

                glEnd();
            }
        }
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
        // Цвет текущего объекта
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

        // Перемещение текущего набора
    case 'w':
    case 'W':
        MoveCurrentTarget(0, MOVE_STEP);
        break;

    case 's':
    case 'S':
        MoveCurrentTarget(0, -MOVE_STEP);
        break;

    case 'a':
    case 'A':
        MoveCurrentTarget(-MOVE_STEP, 0);
        break;

    case 'd':
    case 'D':
        MoveCurrentTarget(MOVE_STEP, 0);
        break;

        // Выбор примитива в режиме редактирования
    case '\t':
        if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
        {
            SelectPreviousPrimitive();
        }
        else
        {
            SelectNextPrimitive();
        }
        break;

        // Создание нового набора
    case ' ':
        StartNewSet();
        break;

        // Удаление последнего или выбранного примитива
    case '\b':
        DeleteCurrentPrimitive();
        break;

        // Удаление последнего набора
    case 'x':
    case 'X':
        DeleteLastSet();
        break;
    }

	UpdateWindowTitle();

	// Запрашивает перерисовку окна через встроенный функционал GLUT
    glutPostRedisplay();
}


// Обработчик специальных клавиш клавиатуры
void SpecialKeyboard(int key, int x, int y)
{
    bool changeLayer = (glutGetModifiers() & GLUT_ACTIVE_CTRL) != 0;

    switch (key)
    {
    case GLUT_KEY_UP:
        if (changeLayer)
        {
            MoveCurrentLayerForward();
        }
        else
        {
            MoveCurrentTarget(0, MOVE_STEP);
        }
        break;

    case GLUT_KEY_DOWN:
        if (changeLayer)
        {
            MoveCurrentLayerBackward();
        }
        else
        {
            MoveCurrentTarget(0, -MOVE_STEP);
        }
        break;

    case GLUT_KEY_LEFT:
        MoveCurrentTarget(-MOVE_STEP, 0);
        break;

    case GLUT_KEY_RIGHT:
        MoveCurrentTarget(MOVE_STEP, 0);
        break;

    case GLUT_KEY_F2:
        ToggleInteractionMode();
        break;

    case GLUT_KEY_HOME:
        if (changeLayer)
        {
            BringCurrentLayerToFront();
        }
        else
        {
            ResetCurrentTargetPosition();
        }
        break;

    case GLUT_KEY_END:
        if (changeLayer)
        {
            SendCurrentLayerToBack();
        }
        break;

    case GLUT_KEY_PAGE_UP:
        if (CurrentMode == EDIT_MODE)
        {
            SelectPreviousPrimitive();
        }
        else
        {
            SelectPreviousSet();
        }
        break;

    case GLUT_KEY_PAGE_DOWN:
        if (CurrentMode == EDIT_MODE)
        {
            SelectNextPrimitive();
        }
        else
        {
            SelectNextSet();
        }
        break;

    case GLUT_KEY_DELETE:
        DeleteLastSet();
        break;
    }

    UpdateWindowTitle();

    // Запрашивает перерисовку окна через встроенный функционал GLUT
    glutPostRedisplay();
}


// Обрабатывает нажатия мыши
void Mouse(int button, int state, int x, int y)
{
    // Обрабатывает только нажатие
    if (state != GLUT_DOWN)
    {
        return;
    }

    // Добавляет вершину по левому клику
    if (button == GLUT_LEFT_BUTTON && CurrentMode == DRAW_MODE)
    {
        AddVertex(x, Height - y);

        UpdateWindowTitle();

        // Запрашивает перерисовку окна
        glutPostRedisplay();
    }
}


// Обрабатывает команды меню
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
        MoveCurrentTarget(0, MOVE_STEP);
        break;

    case MENU_MOVE_DOWN:
        MoveCurrentTarget(0, -MOVE_STEP);
        break;

    case MENU_MOVE_LEFT:
        MoveCurrentTarget(-MOVE_STEP, 0);
        break;

    case MENU_MOVE_RIGHT:
        MoveCurrentTarget(MOVE_STEP, 0);
        break;

    case MENU_PREVIOUS_SET:
        SelectPreviousSet();
        break;

    case MENU_NEXT_SET:
        SelectNextSet();
        break;

    case MENU_LAYER_FORWARD:
        MoveCurrentLayerForward();
        break;

    case MENU_LAYER_BACKWARD:
        MoveCurrentLayerBackward();
        break;

    case MENU_LAYER_FRONT:
        BringCurrentLayerToFront();
        break;

    case MENU_LAYER_BACK:
        SendCurrentLayerToBack();
        break;

    case MENU_TOGGLE_MODE:
        ToggleInteractionMode();
        break;

    case MENU_PREVIOUS_PRIMITIVE:
        SelectPreviousPrimitive();
        break;

    case MENU_NEXT_PRIMITIVE:
        SelectNextPrimitive();
        break;

    case MENU_RESET_POSITION:
        ResetCurrentTargetPosition();
        break;

    case MENU_NEW_SET:
        StartNewSet();
        break;

    case MENU_DELETE_PRIMITIVE:
        DeleteCurrentPrimitive();
        break;

    case MENU_DELETE_ACTIVE_SET:
        DeleteActiveSet();
        break;

    case MENU_DELETE_SET:
        DeleteLastSet();
        break;
    }

    UpdateWindowTitle();

    // Запрашивает перерисовку окна
    glutPostRedisplay();
}


// Подключает меню текущего режима
void AttachContextMenu()
{
    if (DrawMenuId == 0 || EditMenuId == 0)
    {
        return;
    }

    glutDetachMenu(GLUT_RIGHT_BUTTON);
    glutSetMenu(CurrentMode == DRAW_MODE ? DrawMenuId : EditMenuId);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}


// Создаёт контекстные меню режимов
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

    // Подменю порядка слоёв
    int layerMenu = glutCreateMenu(Menu);

    glutAddMenuEntry("Bring forward (Ctrl + Arrow Up)", MENU_LAYER_FORWARD);
    glutAddMenuEntry("Send backward (Ctrl + Arrow Down)", MENU_LAYER_BACKWARD);
    glutAddMenuEntry("Bring to front (Ctrl + Home)", MENU_LAYER_FRONT);
    glutAddMenuEntry("Send to back (Ctrl + End)", MENU_LAYER_BACK);

    // Меню режима рисования
    DrawMenuId = glutCreateMenu(Menu);

    glutAddSubMenu("Set color", colorMenu);
    glutAddSubMenu("Move set", moveMenu);
    glutAddSubMenu("Layer order", layerMenu);
    glutAddMenuEntry("Previous set (Page Up)", MENU_PREVIOUS_SET);
    glutAddMenuEntry("Next set (Page Down)", MENU_NEXT_SET);
    glutAddMenuEntry("Switch to EDIT mode (F2)", MENU_TOGGLE_MODE);
    glutAddMenuEntry("Reset set position (Home)", MENU_RESET_POSITION);
    glutAddMenuEntry("New set (Space)", MENU_NEW_SET);
    glutAddMenuEntry("Delete last primitive (Backspace)", MENU_DELETE_PRIMITIVE);
    glutAddMenuEntry("Delete active set", MENU_DELETE_ACTIVE_SET);
    glutAddMenuEntry("Delete last set (Delete / X)", MENU_DELETE_SET);

    // Меню режима редактирования
    EditMenuId = glutCreateMenu(Menu);

    glutAddSubMenu("Primitive color", colorMenu);
    glutAddSubMenu("Move primitive", moveMenu);
    glutAddSubMenu("Primitive order", layerMenu);
    glutAddMenuEntry("Previous primitive (Page Up / Shift + Tab)", MENU_PREVIOUS_PRIMITIVE);
    glutAddMenuEntry("Next primitive (Page Down / Tab)", MENU_NEXT_PRIMITIVE);
    glutAddMenuEntry("Switch to DRAW mode (F2)", MENU_TOGGLE_MODE);
    glutAddMenuEntry("Reset primitive position (Home)", MENU_RESET_POSITION);
    glutAddMenuEntry("Delete selected primitive (Backspace)", MENU_DELETE_PRIMITIVE);

    AttachContextMenu();
}

int main(int argc, char** argv)
{
    // Создаём первый набор
    Sets.push_back(PrimitiveSet());

    // Инициализируем GLUT
    glutInit(&argc, argv);

	// Настраивает буфер кадра: один буфер и цветовая модель RGB
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

    // Задаёт размер окна
    glutInitWindowSize(Width, Height);

    // Создаёт окно
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

    // Создаёт контекстное меню
    CreateContextMenu();

    // Показывает начальное состояние программы в заголовке
    UpdateWindowTitle();

    // Запускает цикл обработки событий
    glutMainLoop();

    return 0;
}
