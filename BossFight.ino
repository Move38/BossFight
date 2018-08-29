/*
    Boss Fight

    Data structure:
         piece   |   mode    |   value
        ------------------------------
    i.e. PLAYER  |   ATTACK  |     2

    data = 2 bits for piece type, 2 bits for mode, 2 bits for value

    - or -

         piece   |   mode
        ----------------------
    i.e. PLAYER  |   ATTACK2

    data = 2 bits for piece type, 4 bits for mode... (safer for the time being)

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

byte health = 0;
byte attack = 0;

enum pieces {
  BOSS,
  PLAYER,
  RUNE,
  NUM_PIECE_TYPES     //   a handy way to count the entries in an enum
};

byte piece = BOSS;

enum modes {
  STANDBY,
  ATTACK1,      // players and boss can attack at multiple hit points
  ATTACK2,
  ATTACK3,
  INJURED,      // confirm that the hit has been received
  STOCKPILE,    // get those arms
  ARMED,        // let our dealer know we received our arms
  HEAL,         // pop some meds... echinachae tea
  HEALED,       // flaunt your tea
  BOSSFIGHT     // for the waiting period before a boss attacks or heals
};

byte mode = STANDBY;


void setup() {
  Serial.begin();
}

void loop() {

  if ( buttonDoubleClicked() ) {
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
      RUNEMode();
      RUNEDisplay();
      break;
  }

  // share which piece type we are and the mode we are in
  // lower 2 bits communicate the piece type - -
  // upper 4 bits communicate the piece mode - - - -
  byte sendData = mode << 2 + piece;

  setValueSentOnAllFaces(sendData);
}

/*
    Game Logic
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
  return (data == ATTACK1 || data == ATTACK2 || data == ATTACK3);
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
  Game Code
*/

void bossMode() {

  // if the button is pressed and I have my worthy opponents attached, then enter boss fight mode...
  //  if( buttonPressed() ) {
  //    // enter boss fight
  //  }

  if ( mode == STANDBY ) {
    // look at our neighbors
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {

        byte receivedData = getLastValueReceivedOnFace(f);
        byte neigborMode = getModeFromReceivedData(receivedData);
        byte neigborPiece = getPieceFromReceivedData(receivedData);

        //If the RUNE is sending Heal, only Heal the first time Heal is sent
        if (neigborPiece == RUNE && neigborMode == HEAL) {

          health += 1;
          mode = HEALED;  // insures we simple heal once

        }
      }

      //If a Player is attacking you, only take the damage of Attack when it is first sent
      if (neigborPiece == PLAYER && isAttackMode(neigborMode)) {

        // no kicking a dead horse... 0 health is the lowest we go
        if ( health >= getAttackValue(neigborMode)) {
          health -= getAttackValue(neigborMode);
        }
        else {
          health = 0;
        }
        mode = INJURED;
      }
    }
  }

  // do this if we are in attack mode
  if (isAttackMode( mode )) {

  }

  // do this if we are in healed mode
  if (mode == HEALED) {

  }
  // do this if we are in boss fight mode
  if (mode == BOSSFIGHT) {

  }

}

void playerMode() {

  if (buttonPressed()) {
    mode = ATTACK;
  }

  if (isAlone()) {
    mode = STANDBY;
  }

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte receivedData = getLastValueReceivedOnFace(f);

      //Heal the player the first time Heal is sent
      if (getModeFromReceivedData(receivedData) == HEAL) {
        if (prevNeighborModes[f] != HEAL) {
          playerHealth += 1;
        }
      }

      //Add +1 Attack when the RUNE is connected and in Stockpile mode
      if ( getModeFromReceivedData(receivedData) == STOCKPILE) {
        if (prevNeighborModes[f] != STOCKPILE) {
          attack += 1;
        }
      }

      if (mode == ATTACK) {
        //When the Player connects to the Boss, always reduce the attack damage back to 1
        if ( getModeFromReceivedData(receivedData) == BOSSMODE) {
          if (prevNeighborModes[f] != BOSSMODE) {
            attack = 1;
          }
        }
      }
    }
  }

  //Both the PlayerHealth and attack cannot be greater than 3
  if (playerHealth > 3) {
    playerHealth = 3;
  }

  if (attack > 3) {
    attack = 3;
  }

}

void RUNEMode() {

  //Switch between the heal and stockpile state
  if (buttonDoubleClicked()) {
    if (mode == HEAL) {
      mode = STOCKPILE;
    } else if (mode == STOCKPILE) {
      mode = HEAL;
    }
  }

}

/*
  Display Code
*/

void bossDisplay() {
  setColor(OFF);
  FOREACH_FACE(f) {
    if (f < bossHealth) {
      setFaceColor(f, RED);
    }
  }
}

void playerDisplay() {
  setColor(OFF);
  FOREACH_FACE(f) {
    //PlayerHealth is displayed on the left side using faces 0-2
    if (f < playerHealth) {
      setFaceColor(f, GREEN);
    }

    //Attack value is displayed on right side using faces 3-5
    if (f < attack) {
      setFaceColor(5 - f, BLUE);
    }

  }
}

void RUNEDisplay() {
  if (mode == HEAL) {
    setColor(GREEN);
  }

  if (mode == STOCKPILE) {
    setColor(BLUE);
  }
}

