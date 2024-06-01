/* Finn Ferchau | fferc001@ucr.edu:
 * DiscussionSection: 2
 * Assignment: Final Lab Project
 *
 * I acknowledge all content contained herein, excluding
 * template or example code, is my own original work.
 *
 * DemoLink: https://youtu.be/PIpjYblfRpQ
*/

#include <stdlib.h> 
#include <stddef.h> 
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "spiAVR.h"
#include "time.h"
#include "LCD.h"
#include <avr/eeprom.h>

#define NUM_TASKS 6
#define EEPROM_ADDRESS 0
#define GAME_OVER_TONE 500 // Frequency in Hz for game over
#define EAT_TONE 1000      // Frequency in Hz for eating element

// Task struct for concurrent synchSMs implmentations
typedef struct _task
{
  signed char state;         // Task's current state
  unsigned long period;      // Task period
  unsigned long elapsedTime; // Time elapsed since last task tick
  int (*TickFct)(int);       // Task tick function
} task;

// Periods
const unsigned long GCD_PERIOD = 25;    // GCD of all tasks
const unsigned int TASK0_PERIOD = 500;  // Matrix task
const unsigned int TASK1_PERIOD = 25;   // Joystick input task
const unsigned int TASK2_PERIOD = 100;  // LCD display task
const unsigned int TASK3_PERIOD = 200;   // Highscore reset button task
const unsigned int TASK4_PERIOD = 50;  // Buzzer task
const unsigned int TASK5_PERIOD = 25;   // Eat sound task

task tasks[NUM_TASKS]; // declared task array with 5 tasks

void TimerISR()
{
  for (unsigned int i = 0; i < NUM_TASKS; i++)
  { // Iterate through each task in the task array
    if (tasks[i].elapsedTime == tasks[i].period)
    {                                                    // Check if the task is ready to tick
      tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
      tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
    }
    tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
  }
}


// The snakes elements consist of a stuct, that saves their x (row) and y (col) positions.
// These positions are kept in a dynamic list
struct SnakeElement
{
  unsigned char row;
  unsigned char col;
};

struct ListNode
{
  SnakeElement content;
  struct ListNode *next;
};

struct List
{
  struct ListNode *head;
  struct ListNode *tail;
};

void addSnakeHead(List *snake, unsigned char newX, unsigned char newY)
{
  ListNode *newNode = (ListNode *)malloc(sizeof(ListNode));
  if (newNode == NULL)
  {
    return; // malloc error
  }
  newNode->content.col = newX;
  newNode->content.row = newY;
  newNode->next = snake->head;
  snake->head = newNode;
  if (snake->tail == NULL)
  { // Snake was empty
    snake->tail = newNode;
  }
}

void removeSnakeTail(List *snake)
{
  if (snake->head == NULL)
    return; // Empty list, nothing to remove
  if (snake->head == snake->tail)
  {
    // Only one element in the list
    free(snake->head);
    snake->head = snake->tail = NULL;
  }
  else
  {
    ListNode *current = snake->head;
    while (current->next != snake->tail)
    {
      current = current->next;
    }
    free(snake->tail);
    snake->tail = current;
    current->next = NULL;
  }
}

int listLen(struct List *snakeList)
{
  int count = 0;
  struct ListNode *current = snakeList->head;
  while (current != NULL)
  {
    count++;
    current = current->next;
  }
  return count;
}


// Global variables
struct List snakeList = {NULL, NULL}; // Initialize snakeList
enum direction {Up, Down, Right, Left} currentDirection; 
struct SnakeElement currentPos;
struct SnakeElement food;
int score;
int highscore;
bool running;
bool gameOver;
bool eatSound;
bool buzzerInUse;
int prescaler;


// Helper Functions
void resetMatrix()
{
  for (int i = 1; i < 9; i++){
    SPI_SEND_COL(i, 0);
  }
}

void renderSnake(List *snake, SnakeElement *food)
{
  // Loop through snakeList and add all elements to the corresponding row
  unsigned char cols[8] = {0};
  struct ListNode *current = snake->head;
  while (current != NULL)
  {
    cols[current->content.col] |= SetBit(cols[current->content.col], (7 - current->content.row), 1);
    current = current->next;
  }
  // Add food element
  cols[food->col] |= SetBit(cols[food->col], (7 - food->row), 1);

  // Print the rows
  resetMatrix();
  for (int i = 0; i < 8; i++)
  {
    SPI_SEND_COL(i+ 1, cols[i]);
  }
}

bool comparePositions(SnakeElement *pos1, SnakeElement *pos2)
{
  return (pos1->col == pos2->col && pos1->row == pos2->row);
}

struct SnakeElement generateFood(struct List *snakeList)
{
  struct SnakeElement food;
  bool isValidPosition = 0;

  while (!isValidPosition)
  {
    // Generate random position
    food.row = rand() % 8;
    food.col = rand() % 8;

    // Check if this position overlaps with the snake
    isValidPosition = 1; // Assume it's valid until proven otherwise
    struct ListNode *current = snakeList->head;
    while (current != NULL)
    {
      if (comparePositions(&food, &current->content))
      {
        isValidPosition = 0; // Overlaps with snake, not valid
        break;
      }
      current = current->next;
    }
  }

  return food;
}

bool checkCollision(struct List *snakeList, struct SnakeElement *currentPos)
{
  struct ListNode *current = snakeList->head;
  current = current->next;
  while (current != NULL)
  {
    if (comparePositions(currentPos, &current->content))
    {
      return true;
    }
    current = current->next;
  }
  return false;
} 

void resetSnake(struct List *snakeList)
{
  int snakeLen = listLen(snakeList);
  for (int i = 0; i < snakeLen; i++)
  {
    removeSnakeTail(snakeList);
  }
}

void num_to_str(unsigned char number, char *str)
{
  int i = 0;

  if (number == 0)
  {
    str[i++] = '0';
  }
  else
  {
    while (number != 0)
    {
      str[i++] = (number % 10) + '0';
      number /= 10;
    }
  }

  str[i] = '\0';
  int start = 0;
  int end = i - 1;
  char temp;
  while (start < end)
  {
    // Swap characters
    temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

void printHighscoreOnLCD(unsigned char row)
{
  lcd_goto_xy(row, 0);
  lcd_write_str("Highscore:");
  lcd_goto_xy(row, 11);
  char str[4];
  num_to_str(highscore, str);
  lcd_write_str(str);
}

void printScoreOnLCD(unsigned char row)
{
  lcd_goto_xy(row, 0);
  lcd_write_str("Score:");
  lcd_goto_xy(row, 7);
  char str[4];
  num_to_str(score, str);
  lcd_write_str(str);
}

void set_buzzer_frequency(uint16_t frequency)
{
  uint16_t ocr_value = (F_CPU / (2 * 64 * frequency)) - 1;
  OCR1A = ocr_value;
}

void buzzer_on()
{
  TCCR1A |= (1 << COM1A0); // Enable PWM on OC1A (PB1)
}

void buzzer_off()
{
  TCCR1A &= ~(1 << COM1A0); // Disable PWM on OC1A (PB1)
}


enum MatrixStates
{
  Matrix_Init,
  Matrix_Running,
  Marix_Stop
};

int TickMatrix(int state)
{
  switch (state)  // State transitions
  {
  case Matrix_Init:
    state = Marix_Stop;
    break;

  case Marix_Stop:
    if(running)
    {
      state = Matrix_Running;
    }
    break;

  case Matrix_Running:
    if(!running)
    {
      state = Marix_Stop;
    }
    break;
  
  default:
    state = Matrix_Init;
    break;
  }

  switch (state)  // State actions
  {
    case Marix_Stop:

      break;

    case Matrix_Running:
      // Tick snake
      if(currentDirection == Up){
        currentPos.row = (currentPos.row + 1) % 8;
        addSnakeHead(&snakeList, currentPos.col, currentPos.row);
      }
      else if (currentDirection == Down)
      {
        currentPos.row = (currentPos.row - 1 + 8) % 8;
        addSnakeHead(&snakeList, currentPos.col, currentPos.row);
      }
      else if (currentDirection == Right)
      {
        currentPos.col = (currentPos.col + 1) % 8;
        addSnakeHead(&snakeList, currentPos.col, currentPos.row);
      }
      else if (currentDirection == Left)
      {
        currentPos.col = (currentPos.col - 1 + 8) % 8;
        addSnakeHead(&snakeList, currentPos.col, currentPos.row);
      }

      // Check if snake has eaten food
      if(comparePositions(&currentPos, &food))
      {
        eatSound = 1;
        score += 1;
        food = generateFood(&snakeList);
      } else {
        removeSnakeTail(&snakeList);
      }

      // Check if snake has run into itself
      if(checkCollision(&snakeList, &currentPos))
      {
        resetSnake(&snakeList);
        addSnakeHead(&snakeList, 3, 3);
        currentPos = {3, 3};
        food = generateFood(&snakeList);
        currentDirection = Up;
        if (score > highscore)
        {
          highscore = score;
        }
        score = 0;
        eeprom_update_word((uint16_t *)EEPROM_ADDRESS, highscore);
        running = 0;
        gameOver = 1;
      }

      resetMatrix();
      renderSnake(&snakeList, &food);
      break;

    default:
      state = Matrix_Init;
      break;
  }

  return state;
}

// Joystick Task
enum JSStates
{
  JSInit,
  JSWait,
  UpPressed,
  DownPressed,
  CenterPressed,
  LeftPressed,
  RightPressed
};
int Tick_JS(int state)
{
  static const int threshold_up = 750;
  static const int threshhold_down = 250;
  bool ct_pressed = !GetBit(PINC, 2);
  bool up_pressed = (ADC_read(0) > threshold_up);
  bool down_pressed = (ADC_read(0) < threshhold_down);
  bool right_pressed = (ADC_read(1) > threshold_up);
  bool left_pressed = (ADC_read(1) < threshhold_down);

  switch (state)
  { // State transitions
  case JSInit:
    state = JSWait;
    break;

  case JSWait:
    if (ct_pressed)
    {
      running = !running;
      state = CenterPressed;
    }
    else if (up_pressed)
    {
      state = UpPressed;
    }
    else if (down_pressed)
    {
      state = DownPressed;
    } else if (right_pressed)
    {
      state = RightPressed;
    } else if (left_pressed)
    {
      state = LeftPressed;
    }
    break;

  case UpPressed:
    if (!up_pressed)
    {
      state = JSWait;
    }
    break;

  case DownPressed:
    if (!down_pressed)
    {
      state = JSWait;
    }
    break;

  case RightPressed:
    if(!right_pressed){
      state = JSWait;
    }
    break;

  case LeftPressed:
    if(!left_pressed){
      state = JSWait;
    }
    break;

  case CenterPressed:
    if (!ct_pressed)
    {
      state = JSWait;
    }
    break;

  default:
    state = JSInit;
    break;
  }
  switch (state) // State actions
  {
  case JSWait:
    break;

  case UpPressed:
    if((currentDirection == Left || currentDirection == Right) && running)
    {
      currentDirection = Up;
    }
    break;

  case DownPressed:
    if ((currentDirection == Left || currentDirection == Right) && running)
    {
      currentDirection = Down;
    }
    break;

  case RightPressed:
    if ((currentDirection == Up || currentDirection == Down) && running)
    {
      currentDirection = Right;
    }
    break;

  case LeftPressed:
    if ((currentDirection == Up || currentDirection == Down) && running)
    {
      currentDirection = Left;
    }
    break;

  case CenterPressed:
    break;

  default:
    state = JSInit;
    break;
  }

  return state;
}

enum LCDStates {LCD_Init, LCD_NewGame, LCD_InGame, LCD_Pause, LCD_GameOver};
int TickLCD(int state)
{
  static int lastScore;

  switch (state)  // State transitions
  {
    case LCD_Init:
      lastScore = score;
      lcd_clear();
      lcd_goto_xy(0, 0);
      lcd_write_str("New Game");
      printHighscoreOnLCD(1);
      state = LCD_NewGame;
      break;

    case LCD_NewGame:
      if(running)
      {
        lastScore = score;
        lcd_clear();
        printScoreOnLCD(0);
        lcd_goto_xy(1, 0);
        lcd_write_str("Press JS");
        state = LCD_InGame;
      }
      break;

    case LCD_InGame:
      if(gameOver)
      {
        lcd_clear();
        lcd_goto_xy(0, 0);
        lcd_write_str("Game Over");
        printHighscoreOnLCD(1);
        state = LCD_GameOver;
      } 
      else if (!running)
      {
        lcd_clear();
        lcd_goto_xy(0, 0);
        lcd_write_str("Game Paused");
        printHighscoreOnLCD(1);
        state = LCD_Pause;
      }
      break;

    case LCD_Pause:
      if (running)
      {
        lcd_clear();
        printScoreOnLCD(0);
        lcd_goto_xy(1, 0);
        lcd_write_str("Press JS");
        state = LCD_InGame;
      }
      break;

    case LCD_GameOver:
      if(running)
      {
        gameOver = 0;
        state = LCD_InGame;
      }
      break;

    default:
      state = LCD_Init;
      break;
  }

  switch (state)  // State actions
  {
  case LCD_Init:
    break;

  case LCD_NewGame:
    break;

  case LCD_InGame:
    if(lastScore != score)
    {
      lastScore = score;
      lcd_clear();
      printScoreOnLCD(0);
      lcd_goto_xy(1, 0);
      lcd_write_str("Press JS");
    }
    break;

  case LCD_Pause:
    break;

  LCD_GameOver:
    break;

  default:
    break;
  }
  return state;
}

enum HSResetStates {HS_Init, HS_Wait, HS_Pressed};
int Tick_HS(int state)
{
  static int i;
  bool btn_pressed = GetBit(PINC, 3);
  switch (state)  // State transitions
  {
  case HS_Init:
    i = 0;
    state = HS_Wait;
    break;
  
  case HS_Wait:
    if(btn_pressed)
    {
      i = 0;
      highscore = 0;
      eeprom_update_dword((uint32_t *)EEPROM_ADDRESS, highscore);
      state = HS_Pressed;
    }
    break;

  case HS_Pressed:
    if(!btn_pressed && i > 20)
    {
      state = HS_Wait;
    }
    break;

  default:
    state = HS_Init;
    break;
  }

  switch (state)  // State actions
  {
  case HS_Init:
    break;

  case HS_Wait:
    PORTB = SetBit(PORTD, 0, 0);
    break;

  case HS_Pressed:
    if (i % 2)
    {
      PORTB = SetBit(PORTD, 0, 1);
    }
    else 
    {
      PORTB = SetBit(PORTD, 0, 0);
    }
    i++;
    break;

  default:
    state = HS_Init;
    break;
  }

  return state;
}

enum BuzzerStates{Buzzer_Init, Buzzer_Wait};
int TickBuzzer(int state)
{
  switch (state)
  { // State transitions
  case Buzzer_Init:
    state = Buzzer_Wait;
    break;

  case Buzzer_Wait:
    break;

  default:
    break;
  }

  switch (state)
  { // State actions
  case Buzzer_Wait:
    if (buzzerInUse && prescaler > 0)
    {
      // Clear current prescaler settings and configure Timer1 for PWM on OC1A (B1)
      TCCR1A = (1 << COM1A0) | (1 << WGM10); // Toggle OC1A on compare match, Fast PWM 8-bit
      TCCR1B = (1 << WGM12) | prescaler;     // Fast PWM, prescaler set dynamically
    }
    else
    {
      // Turn off the buzzer by disabling the Timer1 clock
      TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);
    }
    break;

  default:
    break;
  }

  return state;
}

enum EatSoundStates {EatSound_Init, EatSound_Off, EatSound_On};
int TickEatSound(int state)
{
  static int i;
  switch (state)  // State transitions
  {
    case EatSound_Init:
      state = EatSound_Off;
      break;

    case EatSound_Off:
      if (eatSound)
      {
        buzzerInUse = 1; 
        prescaler = 64;
        i = 0;
        eatSound = 0;
        state = EatSound_On;
      }
      break;

    case EatSound_On:
      if (i > 10)
      {
        buzzerInUse = 0;
        state = EatSound_Off;
      }
      break;
    
    default:
      state = EatSound_Init;
      break;
  }

  switch (state)
  {
    case EatSound_Init:
      break;

    case EatSound_Off:
      break;

    case EatSound_On:
      i++;
      break;

    default:
      state = EatSound_Init;
      break;
  }

  return state;
}

void hardwareInit()
{
  // OUTPUT (DDR) => 1
  // INPUT (DDR) => 0
  // All of B as output
  DDRB = 0b111111;
  PORTB = 0b000000;

  // All of C as input
  DDRC = 0b000000;
  PORTC = 0b111111;
  
  // All of D as output
  DDRD = 0xFF;
  PORTD = 0x00;

  // Buzzer init
  // Set Timer1 to Fast PWM mode with a prescaler of 64
  // Set waveform generation mode to Fast PWM, 8-bit (WGM12, WGM10)
  TCCR1A |= (1 << WGM10);
  TCCR1B |= (1 << WGM12);

  // Set non-inverting mode for OC1A (clear on compare match, set at BOTTOM)
  TCCR1A |= (1 << COM1A1);
  // Set duty cycle to 50% (128 out of 256)
  OCR1A = 128;

  SPI_INIT();

  MAX7219_init();
  resetMatrix();

  lcd_init();
  ADC_init();
  serial_init(9600);
}

int main(void)
{
  hardwareInit();

  // Seed the random number generator
  srand(time(NULL));

  // Set global variables
  currentDirection = Up;
  addSnakeHead(&snakeList, 3, 3);
  currentPos = {3, 3};
  score = 0;
  highscore = eeprom_read_word((uint16_t *)EEPROM_ADDRESS);
  running = 0;
  gameOver = 0;
  food = generateFood(&snakeList);
  renderSnake(&snakeList, &food);
  buzzerInUse = 0;
  prescaler = 64;


  // Tasks init
  tasks[0].state = Matrix_Init;
  tasks[0].elapsedTime = 0;
  tasks[0].period = TASK0_PERIOD;
  tasks[0].TickFct = &TickMatrix;

  tasks[1].state = JSInit;
  tasks[1].elapsedTime = 0;
  tasks[1].period = TASK1_PERIOD;
  tasks[1].TickFct = &Tick_JS;

  tasks[2].state = LCD_Init;
  tasks[2].elapsedTime = 0;
  tasks[2].period = TASK2_PERIOD;
  tasks[2].TickFct = &TickLCD;

  tasks[3].state = HS_Init;
  tasks[3].elapsedTime = 0;
  tasks[3].period = TASK3_PERIOD;
  tasks[3].TickFct = &Tick_HS;

  tasks[4].state = Buzzer_Init;
  tasks[4].elapsedTime = 0;
  tasks[4].period = TASK4_PERIOD;
  tasks[4].TickFct = &TickBuzzer;

  tasks[5].state = EatSound_Init;
  tasks[5].elapsedTime = 0;
  tasks[5].period = TASK5_PERIOD;
  tasks[5].TickFct = &TickEatSound;

  TimerSet(GCD_PERIOD);
  TimerOn();

  while (1){}
  return 0;
}