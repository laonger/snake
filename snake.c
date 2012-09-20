#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

int one_second = 1000000;

int now_time = 0;
int time_rate = 100;
//int speed = 800000;
int speed = 200000;
int speed_up = 2000;
int speed_max = 50000;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

// 一节身体
typedef struct _body_struct {
    int x, y;
    int i;
    struct _body_struct *pre;   // 前面那节
    struct _body_struct *next;  // 后面那节
} BODY;

// 这是一条蛇
typedef struct _snake_struct {
    char head_chr;      // 脑袋，@
    char body_chr;      // 身子，#
    int length;     // 长度, 初始5
    int d; // KEY_DOWN 0402, KEY_UP 0403, KEY_LEFT 0404, KEY_RIGHT 0405, 
    int x, y;   // 头坐标
    BODY *body_1st;  // 第一个身体节点
    BODY *body_last;  // 最后一个身体节点
} SNAKE;

// 嘎嘎新的一节身体
BODY *new_body();

// 在头后面，加一节身体
void new_body_affer_head(SNAKE *snake);

// 把最后一节身体删掉
void pop_last_body(SNAKE *snake);

// 吃了食物以后，加一节身体
void add_last_body(SNAKE *snake);

// 诞生一条蛇
void init_snake(SNAKE *snake);

// 把蛇从头到尾画出来
void draw_whole_snake(SNAKE *snake);

// 脑袋动一格
void move_head(SNAKE *snake);

// 移动
int snake_move(SNAKE *snake, int *food);

// 小蛇自动跑。。。。
void *auto_run(void *args);

// 来个随机豆儿
int * random_food(SNAKE *snake);

// 画豆儿
void draw_food(int food[]);

// 是否挂掉了
int judge_dead(SNAKE *snake);

int main(int argc, char *argv[])
{
    int ch;

    initscr();
    start_color();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);

    attron(COLOR_PAIR(1));
    //画边
    mvaddch(1, 0, '+');
    mvaddch(1, COLS-1, '+');
    mvaddch(LINES-1, COLS-1, '+');
    mvaddch(LINES-1, 0, '+');
    mvhline(1, 1, '-', COLS-1-1);
    mvhline(LINES-1, 1, '-', COLS-1-1);
    mvvline(2, 0, '|', LINES-2-1);
    mvvline(2, COLS-1, '|', LINES-2-1);

    refresh();
    attroff(COLOR_PAIR(1));

    // 来条蛇
    SNAKE snake;
    init_snake(&snake);
    draw_whole_snake(&snake);
    
    // 初始豆儿
    int *food;
    food = random_food(&snake);
    //mvaddch(food[1], food[0], '*');
    draw_food(food);

    int error;
    pthread_t tidp;
    void * thread_args[2] = { &snake, food };
    error = pthread_create(&tidp, NULL, auto_run, thread_args);
    if (error)
    {
        endwin();
        return 1;
    }
    
    int d;
    while ((ch = getch()) != KEY_F(1))
    {
        if (ch < 258 || ch > 261)
            continue;
        if ( ch+snake.d != KEY_LEFT+KEY_UP )
        {
            d = ch - snake.d;
            if ( d == 1 || d == -1)
                continue;
        }
        pthread_mutex_lock(&mut);
        now_time = 0;
        snake.d = ch;

        if (snake_move(&snake, food))
            break;
        //usleep(speed);
        pthread_mutex_unlock(&mut);

    }
    getch();
    endwin();
    return 0;
}

BODY *new_body()
{
    BODY *b;
    b = malloc(sizeof(BODY));
    b->pre = NULL;
    b->next = NULL;
    return b;
}

void new_body_affer_head(SNAKE *snake)
{
    BODY *new_b = new_body();
    new_b->x = snake->x;
    new_b->y = snake->y;
    new_b->next = snake->body_1st;
    snake->body_1st->pre = new_b;
    snake->body_1st = new_b;
    mvaddch(new_b->y, new_b->x, '#');
}

void pop_last_body(SNAKE *snake)
{
    mvaddch(snake->body_last->y, snake->body_last->x, ' ');
    snake->body_last->pre->next = NULL;
    snake->body_last = snake->body_last->pre;
}

void add_last_body(SNAKE *snake)
{
    snake->length = snake->length+1;
    BODY *new_b = new_body();
    new_b->pre = snake->body_last;
    snake->body_last->next = new_b;

    if (snake->body_last->x == snake->body_last->pre->x)
    {
        new_b->x = snake->body_last->x;
        if (snake->body_last->y > snake->body_last->pre->y)
            new_b->y = snake->body_last->y+1;
        else
            new_b->y = snake->body_last->y-1;
    }
    else
    {
        new_b->y = snake->body_last->y;
        if (snake->body_last->x > snake->body_last->pre->x)
            new_b->x = snake->body_last->x+1;
        else
            new_b->x = snake->body_last->x-1;
    }

    snake->body_last = new_b;
}

void init_snake(SNAKE *snake)
{
    snake->head_chr = '@';
    snake->body_chr = '#';
    snake->length = 7;
    snake->d = KEY_LEFT;
    snake->y = LINES/2;
    snake->x = COLS/2;
    snake->body_1st = NULL;
    
    BODY *pre_b;
    BODY *b;
    int i;
    for (i=0; i<snake->length; i=i+1)
    {
        b = new_body();
        b->i = i;
        b->x = snake->x+i+1;
        b->y = snake->y;
        if (i==0)
        {
            snake->body_1st = b;
        }
        else
        {
            b->pre = pre_b;
            b->next = NULL;
            pre_b->next = b;
        }
        if (i==snake->length-1)
        {
            snake->body_last = b;
        }
        pre_b = b;
    }
}

void draw_whole_snake(SNAKE *snake)
{
    mvaddch(snake->y, snake->x, snake->head_chr);
    BODY *b;
    for (b = snake->body_1st; b != NULL; b = b->next)
    {
        mvaddch(b->y, b->x, snake->body_chr);
    }
    refresh();
}

void move_head(SNAKE *snake)
{
    switch(snake->d)
    {
        case KEY_LEFT:
            snake->x = snake->x-1;
            break;
        case KEY_RIGHT:
            snake->x = snake->x+1;
            break;
        case KEY_UP:
            snake->y = snake->y-1;
            break;
        case KEY_DOWN:
            snake->y = snake->y+1;
            break;
    }
}

int snake_move(SNAKE *snake, int *food)
{
    // 丢掉最后一节身体
    pop_last_body(snake);

    // 在头部加个新身体
    new_body_affer_head(snake);

    // 脑袋前进一下
    move_head(snake);
    mvaddch(snake->y, snake->x, snake->head_chr);

    // 吃豆儿
    if (snake->y == food[1] && snake->x == food[0])
    {
        add_last_body(snake);
            
        food = random_food(snake);
        draw_food(food);
        if (speed - speed_up > speed_max)
        {
            speed = speed - speed_up;
        }
    }

    // 死蛇
    if ((judge_dead(snake)) == 1)
    {
        snake->head_chr = 'X';
        snake->body_chr = 'x';

        init_pair(3, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(3));
        draw_whole_snake(snake);
        mvprintw(LINES/2-1, COLS/2-1, "DEAD");
        refresh();
        attroff(COLOR_PAIR(3));
        return 1;
    }

    mvprintw(0, 0, "snake head at: %d, %d;    snake length: %d;    food at: %d, %d;   ", snake->x, snake->y, snake->length, food[0], food[1]);
    refresh();
    return 0;
}

void *auto_run(void *args)
{
    now_time = 0;
    SNAKE * snake = ((void **)args)[0];
    int * food = ((void **)args)[1];
    while (1)
    {
        usleep(time_rate);
        pthread_mutex_lock(&mut);
        now_time = now_time + time_rate;
        //mvprintw(0, 40, "%d", now_time);
        refresh();
        if (now_time < speed)
        {
            pthread_mutex_unlock(&mut);
            continue;
        }
        now_time = 0;
        if (snake_move(snake, food))
        {
            getch();
            endwin();
            exit(0);
        }
        pthread_mutex_unlock(&mut);
    }
    return (void *) 1;
}

int * random_food(SNAKE *snake)
{
    static int f[2];
    int x;
    int y;
    int yes = 0;
    while(1)
    {
        x = rand()%(COLS-3)+1;
        y = rand()%(LINES-4)+2;
        BODY *b;
        if (x != snake->x && y != snake->y)
        {
            yes = 1;
            for (b = snake->body_1st; b->next != NULL; b = b->next)
            {
                if ( x == b->x && y == b->y )
                {
                    
                    yes = 0;
                    break;
                }
            }
            if (yes==1)
                break;
        }
    }
    f[0] = x;
    f[1] = y;

    return f;
}

void draw_food(int food[])
{
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    attron(COLOR_PAIR(2));
    mvaddch(food[1], food[0], '*');
    attroff(COLOR_PAIR(2));
    refresh();
}

int judge_dead(SNAKE *snake)
{
    if (snake->x >= COLS-1 || snake->y >= LINES-1 || snake->x <= 0 || snake->y < 2)
    {
        return 1;
    }
    BODY *b;
    for (b = snake->body_1st->next->next->next; b->next != NULL; b = b->next)
    {
        if ( snake->x == b->x && snake->y == b->y )
            return 1;
    }
    return 0;
}

