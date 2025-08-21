#include <stdio.h>
#include <string.h>
#include "../include/dll.h"

#define SPADES   0
#define HEARTS   1
#define DIAMONDS 2
#define CLUBS    3
#define NOTRUMP  4

#define NORTH    0
#define EAST     1
#define SOUTH    2
#define WEST     3

#define R2  0x0004
#define R3  0x0008
#define R4  0x0010
#define R5  0x0020
#define R6  0x0040
#define R7  0x0080
#define R8  0x0100
#define R9  0x0200
#define RT  0x0400
#define RJ  0x0800
#define RQ  0x1000
#define RK  0x2000
#define RA  0x4000

void PrintHand(char title[],
               unsigned int remainCards[DDS_HANDS][DDS_SUITS]);
void PrintFut(char title[], futureTricks * fut);
void equals_to_string(int equals, char * res);
int parsePBN(const char* pbn, unsigned int holdings[4][4]);

unsigned char dcardRank[16] =
{
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-' 
};

unsigned char dcardSuit[5] = { 'S', 'H', 'D', 'C', 'N' };

unsigned short int dbitMapRank[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};


int main()
{
  deal dl;
  futureTricks fut;

  unsigned int holdings[4][4];
  
  // Example PBN string (same deal as the hardcoded one)
  const char* pbn = "W:239... J...2J Q...3Q A...4A";
  
  // Parse PBN string to fill holdings array
  if (!parsePBN(pbn, holdings))
  {
    printf("Error: Failed to parse PBN string\n");
    return 1;
  }

  int target;
  int solutions;
  int mode;
  int threadIndex = 0;
  int res;
  char line[80];

  SetMaxThreads(0);

  dl.trump = SPADES;
  dl.first = NORTH;

  dl.currentTrickSuit[0] = 0;
  dl.currentTrickSuit[1] = 0;
  dl.currentTrickSuit[2] = 0;

  dl.currentTrickRank[0] = 0;
  dl.currentTrickRank[1] = 0;
  dl.currentTrickRank[2] = 0;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      dl.remainCards[h][s] = holdings[s][h];

  target = -1; // -1: Solve for the maximum number of tricks possible.
  solutions = 2; // 2: Return all moves with the same score as the best move
  mode = 0;
  res = SolveBoard(dl, target, solutions, mode, &fut, threadIndex);

  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    printf("DDS error: %s\n", line);
    return 1;
  }

  sprintf(line, "Result: \n");

  PrintHand(line, dl.remainCards);

  PrintFut(line, &fut);

  return 0;
}

#define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 12
#define DDS_HAND_LINES 12

void PrintHand(char title[],
               unsigned int remainCards[DDS_HANDS][DDS_SUITS])
{
  int c, h, s, r;
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
  }

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET;
      line = 8;
    }
    else
    {
      offset = 0;
      line = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if ((remainCards[h][s] >> 2) & dbitMapRank[r])
          text[line + s][c++] = static_cast<char>(dcardRank[r]);
      }

      if (c == offset)
        text[line + s][c++] = '-';

      if (h != 3)
        text[line + s][c] = '\0';
    }
  }
  printf("%s", title);
  char dashes[80];
  int l = static_cast<int>(strlen(title)) - 1;
  for (int i = 0; i < l; i++)
    dashes[i] = '-';
  dashes[l] = '\0';
  printf("%s\n", dashes);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    printf("%s\n", text[i]);
  printf("\n\n");
}

void PrintFut(char title[], futureTricks * fut)
{
  printf("%s\n", title);

  printf("%6s %-6s %-6s %-6s %-6s\n",
         "card", "suit", "rank", "equals", "score");

  for (int i = 0; i < fut->cards; i++)
  {
    char res[15] = "";
    equals_to_string(fut->equals[i], res);
    printf("%6d %-6c %-6c %-6s %-6d\n",
           i,
           dcardSuit[ fut->suit[i] ],
           dcardRank[ fut->rank[i] ],
           res,
           fut->score[i]);
  }
  printf("\n");
}


void equals_to_string(int equals, char * res)
{
  int p = 0;
  int m = equals >> 2;
  for (int i = 15; i >= 2; i--)
  {
    if (m & static_cast<int>(dbitMapRank[i]))
      res[p++] = static_cast<char>(dcardRank[i]);
  }
  res[p] = 0;
}

// Parse PBN string and fill holdings array
// PBN format: "N:KQJ6.K652.J85.T9 A973.J97.AT.QJ73 T8.T83.KQ9742.A6 542.AQ4.3.K8542"
// Returns 1 on success, 0 on failure
int parsePBN(const char* pbn, unsigned int holdings[4][4])
{
  if (!pbn || !holdings) return 0;
  
  // Initialize holdings to zero
  for (int h = 0; h < 4; h++)
    for (int s = 0; s < 4; s++)
      holdings[s][h] = 0;
  
  const char* pos = pbn;
  
  // Skip dealer prefix if present (e.g., "N:")
  if (pos[1] == ':')
    pos += 2;
  
  int hand = NORTH;
  int suit = SPADES;
  
  while (*pos && hand < 4)
  {
    char card = *pos;
    
    if (card == ' ')
    {
      // Move to next hand
      hand++;
      suit = SPADES;
      pos++;
      continue;
    }
    else if (card == '.')
    {
      // Move to next suit
      suit++;
      if (suit >= 4)
      {
        // Should not happen in valid PBN
        return 0;
      }
      pos++;
      continue;
    }
    else
    {
      // Parse card rank
      unsigned int rank_mask = 0;
      
      switch (card)
      {
        case 'A': rank_mask = RA; break;
        case 'K': rank_mask = RK; break;
        case 'Q': rank_mask = RQ; break;
        case 'J': rank_mask = RJ; break;
        case 'T': rank_mask = RT; break;
        case '9': rank_mask = R9; break;
        case '8': rank_mask = R8; break;
        case '7': rank_mask = R7; break;
        case '6': rank_mask = R6; break;
        case '5': rank_mask = R5; break;
        case '4': rank_mask = R4; break;
        case '3': rank_mask = R3; break;
        case '2': rank_mask = R2; break;
        case '-': 
          // Empty suit, skip
          pos++;
          continue;
        default:
          // Invalid character
          return 0;
      }
      
      // Add card to holdings
      holdings[suit][hand] |= rank_mask;
      pos++;
    }
  }
  
  return 1;  // Success
}