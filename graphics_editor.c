/*
 * 2D Graphics Editor in C
 * Draws shapes using '*' and '_' on a 2D character canvas.
 * Supports: Circle, Rectangle, Line, Triangle
 * Operations: Add, Delete, Modify objects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─── Canvas dimensions ─────────────────────────────────── */
#define ROWS    30
#define COLS    60
#define MAX_OBJ 50

/* ─── Shape type IDs ─────────────────────────────────────── */
#define SHAPE_CIRCLE    1
#define SHAPE_RECT      2
#define SHAPE_LINE      3
#define SHAPE_TRIANGLE  4

/* ─── Canvas ─────────────────────────────────────────────── */
char canvas[ROWS][COLS];

/* ─── Object store ───────────────────────────────────────── */
typedef struct {
    int  type;          /* SHAPE_* constant              */
    /* Common params: x,y = anchor / centre              */
    int  x, y;
    int  p1, p2;        /* radius | width,height | len,dir | base,height */
    int  active;        /* 1 = in use, 0 = deleted       */
    char ch;            /* fill character: '*' or '_'     */
} Object;

Object objects[MAX_OBJ];
int    obj_count = 0;

/* ══════════════════════════════════════════════════════════
 *  CANVAS UTILITIES
 * ══════════════════════════════════════════════════════════ */

void clear_canvas(void)
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = ' ';
}

void display_picture(void)
{
    /* Top border */
    printf("+");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");

    for (int r = 0; r < ROWS; r++) {
        printf("|");
        for (int c = 0; c < COLS; c++)
            printf("%c", canvas[r][c]);
        printf("|\n");
    }

    /* Bottom border */
    printf("+");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");
}

/* Safe pixel setter – ignores out-of-bounds coordinates */
void set_pixel(int r, int c, char ch)
{
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = ch;
}

/* ══════════════════════════════════════════════════════════
 *  SHAPE DRAWING FUNCTIONS
 * ══════════════════════════════════════════════════════════ */

/* ── Circle (Midpoint algorithm) ──────────────────────── */
void draw_circle(int cx, int cy, int radius, char ch)
{
    int x = 0, y = radius;
    int d = 1 - radius;

    /* Helper: plot 8 symmetric points */
    #define PLOT8(px,py) \
        set_pixel(cy+(py), cx+(px)); set_pixel(cy-(py), cx+(px)); \
        set_pixel(cy+(py), cx-(px)); set_pixel(cy-(py), cx-(px)); \
        set_pixel(cy+(px), cx+(py)); set_pixel(cy-(px), cx+(py)); \
        set_pixel(cy+(px), cx-(py)); set_pixel(cy-(px), cx-(py));

    /* We need ch in scope; wrap in a small lambda-style macro */
    #undef PLOT8
    #define PLOT8(px,py) \
        set_pixel(cy+(py), cx+(px), ch); set_pixel(cy-(py), cx+(px), ch); \
        set_pixel(cy+(py), cx-(px), ch); set_pixel(cy-(py), cx-(px), ch); \
        set_pixel(cy+(px), cx+(py), ch); set_pixel(cy-(px), cx+(py), ch); \
        set_pixel(cy+(px), cx-(py), ch); set_pixel(cy-(px), cx-(py), ch);

    while (x <= y) {
        PLOT8(x, y);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
    #undef PLOT8
}

/* ── Rectangle (outline) ────────────────────────────── */
void draw_rectangle(int r, int c, int width, int height, char ch)
{
    /* Top and bottom edges */
    for (int i = 0; i < width; i++) {
        set_pixel(r,          c + i, ch);
        set_pixel(r + height - 1, c + i, ch);
    }
    /* Left and right edges */
    for (int i = 0; i < height; i++) {
        set_pixel(r + i, c,           ch);
        set_pixel(r + i, c + width - 1, ch);
    }
}

/* ── Line (Bresenham) ───────────────────────────────── */
void draw_line(int r1, int c1, int r2, int c2, char ch)
{
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    int sr = (r1 < r2) ? 1 : -1;
    int sc = (c1 < c2) ? 1 : -1;
    int err = dr - dc;

    while (1) {
        set_pixel(r1, c1, ch);
        if (r1 == r2 && c1 == c2) break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r1 += sr; }
        if (e2 <  dr) { err += dr; c1 += sc; }
    }
}

/* ── Triangle (three vertices) ──────────────────────── */
void draw_triangle(int r1, int c1,
                   int r2, int c2,
                   int r3, int c3, char ch)
{
    draw_line(r1, c1, r2, c2, ch);
    draw_line(r2, c2, r3, c3, ch);
    draw_line(r3, c3, r1, c1, ch);
}

/* ══════════════════════════════════════════════════════════
 *  RENDER ALL ACTIVE OBJECTS ONTO THE CANVAS
 * ══════════════════════════════════════════════════════════ */

void render_all(void)
{
    clear_canvas();
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        Object *o = &objects[i];
        switch (o->type) {
            case SHAPE_CIRCLE:
                draw_circle(o->x, o->y, o->p1, o->ch);
                break;
            case SHAPE_RECT:
                draw_rectangle(o->x, o->y, o->p1, o->p2, o->ch);
                break;
            case SHAPE_LINE:
                /* x,y = start ; p1,p2 = end */
                draw_line(o->x, o->y, o->p1, o->p2, o->ch);
                break;
            case SHAPE_TRIANGLE:
                /* stored as: x=r1,y=c1, p1=r2,p2=c2; extra in a union
                 * For simplicity we compute an equilateral triangle from
                 * apex (x,y), base width p1, height p2 */
                {
                    int apex_r  = o->x,          apex_c  = o->y;
                    int base_r  = o->x + o->p2;
                    int left_c  = o->y - o->p1/2;
                    int right_c = o->y + o->p1/2;
                    draw_triangle(apex_r, apex_c,
                                  base_r, left_c,
                                  base_r, right_c, o->ch);
                }
                break;
        }
    }
}

/* ══════════════════════════════════════════════════════════
 *  OBJECT MANAGEMENT: ADD / DELETE / MODIFY
 * ══════════════════════════════════════════════════════════ */

/* Returns the new object index, or -1 on failure */
int add_object(int type, int x, int y, int p1, int p2, char ch)
{
    if (obj_count >= MAX_OBJ) {
        printf("Object limit reached (%d).\n", MAX_OBJ);
        return -1;
    }
    objects[obj_count] = (Object){ type, x, y, p1, p2, 1, ch };
    printf("Object #%d added.\n", obj_count + 1);
    return obj_count++;
}

void delete_object(int id)   /* id is 1-based */
{
    int i = id - 1;
    if (i < 0 || i >= obj_count || !objects[i].active) {
        printf("No active object with id %d.\n", id);
        return;
    }
    objects[i].active = 0;
    printf("Object #%d deleted.\n", id);
}

void modify_object(int id, int x, int y, int p1, int p2, char ch)
{
    int i = id - 1;
    if (i < 0 || i >= obj_count || !objects[i].active) {
        printf("No active object with id %d.\n", id);
        return;
    }
    objects[i].x  = x;
    objects[i].y  = y;
    objects[i].p1 = p1;
    objects[i].p2 = p2;
    objects[i].ch = ch;
    printf("Object #%d modified.\n", id);
}

/* ══════════════════════════════════════════════════════════
 *  LIST ALL OBJECTS
 * ══════════════════════════════════════════════════════════ */

void list_objects(void)
{
    const char *names[] = { "", "Circle", "Rectangle", "Line", "Triangle" };
    int any = 0;
    printf("\n%-4s %-10s %-5s %-5s %-5s %-5s %s\n",
           "ID", "Shape", "X", "Y", "P1", "P2", "Char");
    printf("----------------------------------------------\n");
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        printf("%-4d %-10s %-5d %-5d %-5d %-5d %c\n",
               i + 1,
               names[objects[i].type],
               objects[i].x, objects[i].y,
               objects[i].p1, objects[i].p2,
               objects[i].ch);
        any = 1;
    }
    if (!any) printf("(no objects)\n");
    printf("\n");
}

/* ══════════════════════════════════════════════════════════
 *  INTERACTIVE MENU
 * ══════════════════════════════════════════════════════════ */

static void print_menu(void)
{
    printf("\n+================================+\n");
    printf("|   2D ASCII Graphics Editor     |\n");
    printf("+================================+\n");
    printf("|  1. Display picture            |\n");
    printf("|  2. Add Circle                 |\n");
    printf("|  3. Add Rectangle              |\n");
    printf("|  4. Add Line                   |\n");
    printf("|  5. Add Triangle               |\n");
    printf("|  6. Delete object              |\n");
    printf("|  7. Modify object              |\n");
    printf("|  8. List objects               |\n");
    printf("|  9. Clear all                  |\n");
    printf("|  0. Exit                       |\n");
    printf("+================================+\n");
    printf("Choice: ");
}

static char get_char_input(void)
{
    char ch;
    printf("Fill character (* or _): ");
    scanf(" %c", &ch);
    if (ch != '*' && ch != '_') ch = '*';
    return ch;
}

int main(void)
{
    int  choice;
    int  x, y, p1, p2, id;
    char ch;

    clear_canvas();

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue; }

        switch (choice) {

        /* ── 1. Display ──────────────────────────────── */
        case 1:
            render_all();
            display_picture();
            break;

        /* ── 2. Add Circle ───────────────────────────── */
        case 2:
            printf("Centre row (0-%d): ", ROWS-1);  scanf("%d", &y);
            printf("Centre col (0-%d): ", COLS-1);  scanf("%d", &x);
            printf("Radius: ");                      scanf("%d", &p1);
            ch = get_char_input();
            add_object(SHAPE_CIRCLE, x, y, p1, 0, ch);
            render_all();
            display_picture();
            break;

        /* ── 3. Add Rectangle ────────────────────────── */
        case 3:
            printf("Top-left row (0-%d): ", ROWS-1); scanf("%d", &y);
            printf("Top-left col (0-%d): ", COLS-1); scanf("%d", &x);
            printf("Width  (cols): ");               scanf("%d", &p1);
            printf("Height (rows): ");               scanf("%d", &p2);
            ch = get_char_input();
            add_object(SHAPE_RECT, y, x, p1, p2, ch);
            render_all();
            display_picture();
            break;

        /* ── 4. Add Line ─────────────────────────────── */
        case 4:
            printf("Start row: "); scanf("%d", &y);
            printf("Start col: "); scanf("%d", &x);
            printf("End   row: "); scanf("%d", &p1);
            printf("End   col: "); scanf("%d", &p2);
            ch = get_char_input();
            add_object(SHAPE_LINE, y, x, p1, p2, ch);
            render_all();
            display_picture();
            break;

        /* ── 5. Add Triangle ─────────────────────────── */
        case 5:
            printf("Apex row (0-%d): ", ROWS-1);     scanf("%d", &y);
            printf("Apex col (0-%d): ", COLS-1);     scanf("%d", &x);
            printf("Base width (cols): ");            scanf("%d", &p1);
            printf("Height     (rows): ");            scanf("%d", &p2);
            ch = get_char_input();
            add_object(SHAPE_TRIANGLE, y, x, p1, p2, ch);
            render_all();
            display_picture();
            break;

        /* ── 6. Delete ───────────────────────────────── */
        case 6:
            list_objects();
            printf("Object ID to delete: "); scanf("%d", &id);
            delete_object(id);
            render_all();
            display_picture();
            break;

        /* ── 7. Modify ───────────────────────────────── */
        case 7:
            list_objects();
            printf("Object ID to modify: ");         scanf("%d", &id);
            printf("New X (col / cx): ");            scanf("%d", &x);
            printf("New Y (row / cy): ");            scanf("%d", &y);
            printf("New P1 (radius/width/endR/baseW): "); scanf("%d", &p1);
            printf("New P2 (0/height/endC/height):   "); scanf("%d", &p2);
            ch = get_char_input();
            modify_object(id, y, x, p1, p2, ch);
            render_all();
            display_picture();
            break;

        /* ── 8. List ─────────────────────────────────── */
        case 8:
            list_objects();
            break;

        /* ── 9. Clear all ────────────────────────────── */
        case 9:
            obj_count = 0;
            clear_canvas();
            printf("All objects cleared.\n");
            display_picture();
            break;

        /* ── 0. Exit ─────────────────────────────────── */
        case 0:
            printf("Goodbye!\n");
            return 0;

        default:
            printf("Invalid choice.\n");
        }
    }
}
