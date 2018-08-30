/*
    Boss Fight

    Work together to remain safe while defeating a boss,
    only one player gets to claim victory.

    Each turn, a player is given the option to:
    1. attack the boss
    2. heal herself
    3. heal another
    4. stockpile arms
    5. gift arms to another

    Each round, the players face their possible fate as they face off
    against the boss that might heal itself or deal the damage.

    Data structure:
         piece   |   mode
        ----------------------
    i.e. PLAYER  |   ATTACK2

    data = 2 bits for piece type, 4 bits for mode...

    by Jusin Ha
    08.29.2018
*/
#include "Serial.h"

ServicePortSerial Serial;     // HELP US! We need some sign of life... or at least text debugging


#define BOSS_MAX_HEALTH      6      //
#define BOSS_START_HEALTH    3      //
#define PLAYER_MAX_HEALTH    3      //
#define PLAYER_START_HEALTH  3      // 
#define PLAYER_MAX_ATTACK    3      // the apprentice becomes the master
#define PLAYER_START_ATTACK  1      // we have little fighting experience

#define BOSS_PROB_HEAL       1      //  ratio for bossFight option
#define BOSS_PROB_FIGHT      1      //
#define BOSS_BUFFED          1      // Make the boss do +1 more damage 

#define PLAYER_ATTACK_TIMEOUT  4000 // apparently we didn't want to fight in the first place
Timer attackTimer;

#define RUNE_TIMEOUT  4000 // apparently we didn't want to fight in the first place
Timer runeTimer;

byte health = 0;
byte attack = 0;
byte injuryValue = 0;

bool attackSuccessful = false;

bool bossBuffed; 

enum pieces {
  BOSS,
  PLAYER,
  RUNE,
  NUM_PIECE_TYPES     //   a handy way to count the entries in an enum
};

byte piece;

enum modes {
  STANDBY,
  ATTACK1,      // players and boss can attack at multiple hit point
  ATTACK2,
  ATTACK3,
  ATTACKBUFF,  // Buffed attack of Boss
  INJURED,      // confirm that the hit has been received
  STOCKPILE,    // get those arms
  ARMED,        // let our dealer know we received our arms
  HEAL,         // pop some meds... echinachae tea
  HEALED,       // flaunt your tea
  BOSSFIGHT,    // for the waiting period before a boss attacks or heals
  DEAD          // if our health is 0 as a player
};

byte mode = STANDBY;


void setup() {
  Serial.begin();
  setPieceType(PLAYER); // start us out as a PLAYAAAA
}

void loop() {

  if ( buttonLongPressed() ) {
    byte nextPiece = (piece + 1) % NUM_PIECE_TYPES;
    setPieceType( nextPiece );
  }

  //When a piece is given one of these roles, assign the logic and display code
  switch (piece) {

    case BOSS:
      bossMode();
      bossDisplay();
      break;

    case PLAYER:
      playerMode();
      playerDisplay();
      break;

    case RUNE:
      runeMode();
      runeDisplay();
      break;
  }

  // share which piece type we are and the mode we are in
  // lower 2 bits communicate the piece type - -
  // upper 4 bits communicate the piece mode - - - -
  byte sendData = (mode << 2) + piece;

  setValueSentOnAllFaces(sendData);
}

/*
    Game Convenience Functions
*/

void setPieceType( byte type ) {

  piece = type;

  switch (piece) {

    case BOSS:
      health = BOSS_START_HEALTH;
      mode = STANDBY;
      break;

    case PLAYER:
      health = PLAYER_START_HEALTH;
      attack = PLAYER_START_ATTACK;
      mode = STANDBY;
      break;

    case RUNE:
      mode = STANDBY;
      break;
  }
}

//Call these formulas when we want to separate the sent data
byte getModeFromReceivedData(byte data) {
  byte mode = (data >> 2) & 15;  // keep only the 4 bits of info
  Serial.print("mode from received data: ");
  Serial.println(mode);
  return mode;
}

byte getPieceFromReceivedData(byte data) {
  byte piece = data & 3;  // keep only the lower 2 bits of info
  Serial.print("piece from received data: ");
  Serial.println(piece);
  return piece;
}

bool isAttackMode (byte data) {
  return (data == ATTACK1 || data == ATTACK2 || data == ATTACK3 || data == ATTACKBUFF);
}

byte getNumberOfNeighbors() {
  byte numNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      numNeighbors++;
    }
  }
  return numNeighbors;
}

byte getAttackValue( byte data ) {
  byte value = 0;

  switch (data) {
    case ATTACK1: value = 1; break;
    case ATTACK2: value = 2; break;
    case ATTACK3: value = 3; break;
  }

  return value;
}


/*
   BOSS LOGIC
*/

void bossMode() {
  // TODO: boss fight mode

  //   if the button is pressed and I have my worthy opponents attached, then enter boss fight mode...
  if ( buttonPressed() ) {
    // enter boss fight
    if (!isAlone()) {
      // random chance that we heal ourselves or dole out the pain
      byte diceRoll = rand(BOSS_PROB_HEAL + BOSS_PROB_FIGHT);
      if ( diceRoll > BOSS_PROB_HEAL && bossBuffed == false) {
        // LET'S FIGHT
       mode = ATTACK1; 
      } else if (diceRoll > BOSS_PROB_HEAL && bossBuffed == true){
        mode = ATTACKBUFF;
      }
      else {
        // DRINK SOME TEA AND REST UP
        mode = HEALED;
      }
    }
    else {
      // no fight to be had
    }
  }

  bool injuredAllNeighbors = true;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      byte receivedData = getLastValueReceivedOnFace(f);
      byte neighborMode = getModeFromReceivedData(receivedData);
      byte neighborPiece = getPieceFromReceivedData(receivedData);


      if ( mode == STANDBY ) {
        //If the RUNE is sending Heal, only Heal the first time Heal is sent
        if (neighborPiece == RUNE && neighborMode == HEAL) {

          mode = HEALED;  // insures we simple heal once
        }

        if (neighborPiece == RUNE && neighborMode == STOCKPILE){
          mode = ARMED;
        }

        //If a Player is attacking you, only take the damage of Attack when it is first sent
        if (neighborPiece == PLAYER && isAttackMode(neighborMode)) {
          // only allow a player to injur the boss when attacking alone, no ganging up
          if (getNumberOfNeighbors() == 1) {
            injuryValue = getAttackValue(neighborMode);
            mode = INJURED;
          }
        }
      }
      // do this if we are in attack mode
      else if (isAttackMode( mode )) {
        // see if we injured all of our neighbors, when we did, switch back to standby
        if (neighborPiece == PLAYER && neighborMode != INJURED) {
          injuredAllNeighbors = false;
        }
      }

      // do this if we are in healed mode (this includes when in a bossfight)
      else if (mode == HEALED) {
        if ((neighborPiece == RUNE || neighborPiece == PLAYER) && neighborMode == STANDBY) {

          health += 1;

          if (health > BOSS_MAX_HEALTH) {
            health = BOSS_MAX_HEALTH;
          }

          mode = STANDBY;
        }
      }

      else if (mode == ARMED){
        bossBuffed = true;
        mode = STANDBY; 
      }

      else if (mode == INJURED) {
        if (neighborPiece == PLAYER && neighborMode == STANDBY) {

          // no kicking a dead horse... 0 health is the lowest we go
          if ( health >= injuryValue ) {
            health -= injuryValue;
            injuryValue = 0;  // not necessary but a good piece of mind
          }
          else {
            health = 0;
          }
          mode = STANDBY;
        }
      }
    }
  }

  // only enter standby afer we have injured all of our neighbors...
  if ( isAttackMode( mode )) {
    if (injuredAllNeighbors) {
      mode = STANDBY;
    }
  }

  // do this if we are in boss fight mode
  if (mode == BOSSFIGHT) {

  }

  // did we get defeated?
  if (health == 0) {
    mode = DEAD;
  }
}

/*
   PLAYER LOGIC
*/
void playerMode() {

  if (buttonPressed()) {
    switch (attack) {
      case 1:     mode = ATTACK1;   break;
      case 2:     mode = ATTACK2;   break;
      case 3:     mode = ATTACK3;   break;
      default:    Serial.println("how did we get here?"); break;
    }
    attackTimer.set(PLAYER_ATTACK_TIMEOUT);
    attackSuccessful = false;
  }

  if (isAlone()) {
    // time out
    if (attackTimer.isExpired()) {
      mode = STANDBY;
      attackTimer.set(NEVER);
      attackSuccessful = false;
    }
  }

  FOREACH_FACE(f) {

    if (!isValueReceivedOnFaceExpired(f)) {

      byte receivedData = getLastValueReceivedOnFace(f);
      byte neighborMode = getModeFromReceivedData(receivedData);
      byte neighborPiece = getPieceFromReceivedData(receivedData);

      if (mode == STANDBY) {

        if ( neighborPiece == RUNE && neighborMode == HEAL ) {
          mode = HEALED;  // let the healer know we got their good deed
        }
        else if (neighborPiece == RUNE && neighborMode == STOCKPILE) {
          mode = ARMED;
        }
        // TODO: handle the boss hitting us
        else if (neighborPiece == BOSS && isAttackMode(neighborMode)) {
          if (neighborMode == ATTACK1) {
            injuryValue = attack;
          } else if (neighborMode == ATTACKBUFF) {
            injuryValue = attack + 1;
          }
          mode = INJURED;
        }
      }
      else if (isAttackMode(mode)) {
        if ( neighborPiece == BOSS && neighborMode == INJURED) {

          if (attackSuccessful) {
            mode = STANDBY;
            // always return our attack to 1
            attack = 1;
          }
          attackSuccessful = true;
        }
      }
      else if (mode == ARMED) {
        if (neighborPiece == RUNE && neighborMode == STANDBY) {
          attack += 1;
          if (attack > PLAYER_MAX_ATTACK) {
            attack = PLAYER_MAX_ATTACK;
          }
          mode = STANDBY;
        }
      }
      else if (mode == HEALED) {
        if (neighborPiece == RUNE && neighborMode == STANDBY) {
          health += 1;
          if (health > PLAYER_MAX_HEALTH) {
            health = PLAYER_MAX_HEALTH;
          }
          mode = STANDBY;
        }
      }
      else if (mode == INJURED) {
        if (neighborPiece == BOSS && neighborMode == STANDBY) {
          if ( health >= injuryValue ) {
            health -= injuryValue;
            injuryValue = 0;  // not necessary but a good piece of mind
          }
          else {
            health = 0;
          }
          mode = STANDBY;
        }
      }
    }
  } // end of loop for looking at neighbors

  // if we are at zero health we are dead
  if (health == 0) {
    mode = DEAD;
  }

  // respawn when we pull our piece away
  if ( mode == DEAD && isAlone() ) {
    setPieceType(PLAYER);
  }
}

/*
   RUNE LOGIC
*/
void runeMode() {

  //charge up to be a healer or a stockpiler... shotcaller, baller...
  if (buttonPressed()) {
    if ( mode == STANDBY ) {
      mode = HEAL;
    }
    else {
      // cycle through our options
      mode = nextRuneMode(mode);
    }
  }

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      byte receivedData = getLastValueReceivedOnFace(f);
      byte neighborMode = getModeFromReceivedData(receivedData);
      byte neighborPiece = getPieceFromReceivedData(receivedData);

      if (mode == STANDBY) {
        // nothing to see here
      }
      else if (mode == HEAL) {
        // return to standby if we see that we successfully healed our neighbor

        if ( (neighborPiece == PLAYER || neighborPiece == BOSS) && neighborMode == HEALED) {
          mode = STANDBY;
        }
      }
      else if (mode == STOCKPILE) {
        // return to standby if we see that we successfully armed our neighbor

        if ( (neighborPiece == PLAYER  || neighborPiece == BOSS) && neighborMode == ARMED) {
          mode = STANDBY;
        }

      }
    }
  }
}

byte nextRuneMode( byte data) {
  byte nextMode = STANDBY;
  switch ( data ) {
    case HEAL:      nextMode = STOCKPILE; break;
    case STOCKPILE: nextMode = HEAL;      break;
  }
  return nextMode;
}

/*
  Display Code
*/

void bossDisplay() {
  setColor(OFF);
  FOREACH_FACE(f) {
    if (f < health) {
      setFaceColor(f, dim(RED, 127));
    }
  }
  if (mode == INJURED) {
    setFaceColor(5, YELLOW);
  }

  if (mode == DEAD) {
    // something red
    setFaceColor(rand(6), dim(RED, rand(255)));
  }

  if (mode == ATTACKBUFF) {
    setFaceColor(0, MAGENTA);
  }
}

void playerDisplay() {
  setColor(OFF);
  FOREACH_FACE(f) {
    //PlayerHealth is displayed on the left side using faces 0-2
    if (f < health) {
      setFaceColor(f, GREEN);
    }
    //Attack value is displayed on right side using faces 3-5
    if (f < attack) {
      setFaceColor(5 - f, ORANGE);
    }
  }
  if (isAttackMode(mode)) {
    // flash white
    if ((millis() / 500) % 2 == 0) {
      setFaceColor(0, WHITE);
    }
  }

  if (mode == DEAD) {
    // flash blue and green
    if ((millis() / 500) % 2 == 0) {
      setColor(dim(BLUE, 127));
    }
    else {
      setColor(dim(GREEN, 127));
    }
  }
}

void runeDisplay() {
  if (mode == HEAL) {
    setColor(GREEN);
  }
  else if (mode == STOCKPILE) {
    setColor(ORANGE);
  }
  else {  // STANDBY
    setColor(dim(WHITE, 127));
  }

}

