/******************************
 * vier.c    "4 gewinnt" Spiel
 ******************************/

/*Verbessern:
 *
 * Spielfeld wird immer "gecleart"
 * CHCECK Spielfeld mit seitlichen Strichen 
 *
 */


#include <stdio.h>
#include <string.h>
 
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


/*** functions ***/
 
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

// Spielstein in Spalte versenken
int drop_disc(int gamer, int column) 
{
    int row;

    for (row = 0; row < ROWS; row++)
    {
        if(Grid[row][column-1] > 0) break;
    }

    if (row == 0) return 0;

    Grid[row-1][column-1] = gamer;

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
int search_four(int gamer) 
{
    int i, j;
     
    // Suche waagrecht    
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS - 3; j++)
        {
            if (Grid[i][j] == gamer && Grid[i][j+1] == gamer && Grid[i][j+2] == gamer && Grid[i][j+3] == gamer ) 
            {
                mark_four(i, j, 0, +1);
                return gamer; 
            }
        }
    }

    // Suche senkrecht    
    for (j=0; j<COLS; j++)
    {    
        for (i=0; i<ROWS-3; i++)
        {
            if (Grid[i][j]   == gamer && Grid[i+1][j] == gamer && Grid[i+2][j] == gamer && Grid[i+3][j] == gamer ) 
            {
                mark_four(i, j, +1, 0);
                return gamer; 
            }
        }
    }

    // Suche diagonal '\'   
    for(i=0; i<ROWS-3; i++)
    {
        for(j=0; j<COLS-3; j++)
        {
            if(Grid[i][j] == gamer && Grid[i+1][j+1] == gamer && Grid[i+2][j+2] == gamer && Grid[i+3][j+3] == gamer )
            {
                mark_four(i, j, +1, +1);
                return gamer; 
            }
        }
    }

    // Suche diagonal '/'
    for (i=0; i<ROWS-3; i++)
    {
        for (j=COLS-4; j<COLS; j++)
        {
            if (Grid[i][j] == gamer && Grid[i+1][j-1] == gamer && Grid[i+2][j-2] == gamer && Grid[i+3][j-3] == gamer)
            {
                mark_four(i, j, +1, -1);
                return gamer;
            }
        }
    }

    return 0;
}
 
// Spaltennummer vom Spieler abfragen
int get_input_col(int gamer) 
{
    int column;
    int input_valid = 0;
     
    while (!input_valid) 
    {
        printf("Spieler %d ('%c') setzt Spalte (1-%d): ", gamer, Symbols[gamer], COLS);
        scanf("%d", &column);
        if (column < 1 || column > COLS)
            printf("Ungültige Spaltennummer.\n"); 
        else
            input_valid = 1;
    }
    return column;
}
     
/*** main ***/
int main(void) 
{
    int discs = (ROWS * COLS);
    int column, gamer, winner;

    memset(Grid, 0, sizeof(Grid));

    printf("\e[1;1H\e[2J"); 

    show_grid();

    gamer = 1;

    while (discs > 0) 
    {
        column = get_input_col(gamer);

        if (drop_disc(gamer, column)) discs--;
        else 
        { 
            printf("Kein freier Platz in Spalte %d.\n", column);
            continue;
        }

        winner = search_four(gamer);

        printf("\e[1;1H\e[2J"); 

        show_grid();
        
        if (winner > 0) 
        {
            printf("Gewinner ist Spieler %d (%c). \n", winner, Symbols[gamer]);
            break;
        }

        gamer = gamer%2+1;
    }
     

    if(discs == 0) printf("Das Spielfeld ist voll!\n  Unentschieden!\n");
        
    printf("%sGAME OVER\n", INDENT);
    return 0;
} 
