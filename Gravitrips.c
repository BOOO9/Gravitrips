/******************************
 * vier.c    "4 gewinnt" Spiel
 ******************************/

/*Verbessern:
 *
 * Error checker für Eingabe
 * 
 *
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

 
#define ROWS 6
#define COLS 7
#define INDENT "    "
#define PRLF   printf("\n")
 
/*  Symbole: 'o' = Stein Spieler 1
 *           'x' = Stein Spieler 2  
 *           'O' = Vierer-Reihe Spieler 1
 *           'X' = Vierer-Reihe Spieler 2  
*/

char Symbols[] = {' ', 'O', 'X', '4', '4'};
 
// Spielstand wird im Grid Array gespeichert. 
// Die Spielernummer wird im Feld gespeichert in dem der Stein ligen sollte

int Grid[ROWS][COLS];

//  Spielmatrix: 0 = freies Feld
//    (Grid)     1 = Stein Spieler 1
//               2 = Stein Spieler 2  
//               3 = Vierer-Reihe Spieler 1
//               4 = Vierer-Reihe Spieler 2  


void show_grid(void);
int drop_disc(int player, int column);
void mark_four(int rs, int cs, int dr, int dc);
int search_winner(int player);
int get_input_col(int player); 

int main(void) 
{
    int discs = (ROWS * COLS);
    int column, player, winner;
    
    //set whole Grid on 0
    memset(Grid, 0, sizeof(Grid));
    
    //clears screen
    printf("\e[1;1H\e[2J"); 


    show_grid();

    player = 1;

    while (discs > 0) 
    {
        column = get_input_col(player);

        if (drop_disc(player, column) == 1) discs--;
        else 
        { 
            printf("Kein freier Platz in Spalte %d.\n", column);
            continue;
        }

        winner = search_winner(player);

        printf("\e[1;1H\e[2J"); 

        show_grid();
        
        if (winner > 0) 
        {
            printf("Gewinner ist Spieler %d (%c). \n", winner, Symbols[player]);
            break;
        }

        player = player%2+1;
    }
     

    if(discs == 0) printf("Das Spielfeld ist voll!\n  Unentschieden!\n");
        
    printf("%sGAME OVER\n", INDENT);
    return 0;
}



/* Functions */
 
// Show Grid
void show_grid(void) 
{
    int i,j;

    PRLF;
    
    printf("%s   ", INDENT);
    
    for (j = 1; j <= COLS; j++)
        printf("|%d", j);
    
    printf("|");

    //printf("\n%s  ----------------", INDENT);

    PRLF;
    
    for (i = 0; i < ROWS; i++) 
    {
        
        printf("%s%d  ", INDENT, i+1);

        for (j = 0; j < COLS; j++)
            printf("|%c", Symbols[Grid[i][j]]);

        printf("|");
        PRLF;

    }

    PRLF;
}

// Returns 0 if col is full
int drop_disc(int player, int column) 
{
    int row;

    for (row = 0; row < ROWS; row++)
    {
        if(Grid[row][column-1] > 0) break;
    }

    if (row == 0) return 0;

    Grid[row-1][column-1] = player;

    return 1;
}   

// Vierer-Reihe markieren
void mark_four(int rs, int cs, int dr, int dc)
{
    int i;

    for (i=0; i<4; i++) 
        Grid[rs+i*dr][cs+i*dc] += 2;
}
 
// Vierer-Reihe suchen
int search_winner(int player) 
{
    int i, j;
     
    // Suche waagrecht    
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS - 3; j++)
        {
            if (Grid[i][j] == player && Grid[i][j+1] == player && Grid[i][j+2] == player && Grid[i][j+3] == player ) 
            {
                mark_four(i, j, 0, +1);
                return player; 
            }
        }
    }

    // Suche senkrecht    
    for (j=0; j<COLS; j++)
    {    
        for (i=0; i<ROWS-3; i++)
        {
            if (Grid[i][j]   == player && Grid[i+1][j] == player && Grid[i+2][j] == player && Grid[i+3][j] == player ) 
            {
                mark_four(i, j, +1, 0);
                return player; 
            }
        }
    }

    // Suche diagonal '\'   
    for(i=0; i<ROWS-3; i++)
    {
        for(j=0; j<COLS-3; j++)
        {
            if(Grid[i][j] == player && Grid[i+1][j+1] == player && Grid[i+2][j+2] == player && Grid[i+3][j+3] == player )
            {
                mark_four(i, j, +1, +1);
                return player; 
            }
        }
    }

    // Suche diagonal '/'
    for (i=0; i<ROWS-3; i++)
    {
        for (j=COLS-4; j<COLS; j++)
        {
            if (Grid[i][j] == player && Grid[i+1][j-1] == player && Grid[i+2][j-2] == player && Grid[i+3][j-3] == player)
            {
                mark_four(i, j, +1, -1);
                return player;
            }
        }
    }

    return 0;
}
 
// Spaltennummer vom Spieler abfragen
int get_input_col(int player) 
{
    int column;
    int input_valid = 1;
     
    while (input_valid == 1) 
    {
        printf("Spieler %d ('%c') setzt Spalte (1-%d): ", player, Symbols[player], COLS);
        scanf("%d", &column);
        
        if (column < 1 || column > COLS)
            printf("Ungültige Spaltennummer.\n"); 
        else
            input_valid = 0; 
    }
    return column;
}

