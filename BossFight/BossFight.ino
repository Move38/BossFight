
byte bossHealth = 3;

byte attack = 0;
byte playerHealth = 1;

enum pieces {
  BOSS,
  PLAYER,
  GAMEMANAGER
};

byte piece = BOSS;

enum modes {
  STANDBY,
  ATTACK,
  STOCKPILE,
  HEAL,
  BOSSMODE
};

byte mode = STANDBY;

byte prevNeighborModes[6];

#include "Serial.h"

ServicePortSerial Serial;

void setup() {

  Serial.begin();

  // put your setup code here, to run once:
  piece = PLAYER;

  //At the start of the game, the Boss will always send out that it is in Bossmode
  if (piece == BOSS){
    mode = BOSSMODE;
  }

  //At the start of the game, the Player will always send out that it is attacking and have an attack of 1
  if (piece == PLAYER) {
    mode = ATTACK;
    attack = 1;
  }

  //The GameManager will always start out in heal mode
  if (piece == GAMEMANAGER) {
    mode = HEAL;
  }

}

void loop() {

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

    case GAMEMANAGER:

      gameManagerMode();
      gameManagerDisplay();
      break;

  }

  //Put piece mode and attack into one byte value to send out
  byte sendData = (mode * 10) + attack;

  setValueSentOnAllFaces(sendData);

  //Create an array of previous data on each face so we can compare it when receiving data later
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte receivedData = getLastValueReceivedOnFace(f);
      prevNeighborModes[f] = getModeFromReceivedData(receivedData);
    } else {
      prevNeighborModes[f] = STANDBY;
    }
  }
}

//Call these formulas when we want to separate the sent data
byte getModeFromReceivedData(byte data) {
  return (data / 10);
}

byte getAttackFromReceivedData(byte data) {
  return (data % 10);
}

/*
  Game Code
*/

void bossMode() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte receivedData = getLastValueReceivedOnFace(f);

      //If the GameManager is sending Heal, only Heal the first time Heal is sent
      if (getModeFromReceivedData(receivedData) == HEAL) {
        if (prevNeighborModes[f] != HEAL) {
          bossHealth += 1;
        }
      }

      //If a Player is attacking you, only take the damage of Attack when it is first sent
      if (getModeFromReceivedData(receivedData) == ATTACK) {
        if (prevNeighborModes[f] != ATTACK) {
          bossHealth -= getAttackFromReceivedData(receivedData);
        }
      }
    }

    //The Boss's health cannot be more than 6 or less than 0
    if (bossHealth > 6) {
      bossHealth = 6;
    } else if (bossHealth <= 0) {
      bossHealth = 0;
    }

  }
}

void playerMode() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte receivedData = getLastValueReceivedOnFace(f);

      //Heal the player the first time Heal is sent
      if (getModeFromReceivedData(receivedData) == HEAL) {
        if (prevNeighborModes[f] != HEAL) {
          playerHealth += 1;
        }
      }

      //Add +1 Attack when the GameManager is connected and in Stockpile mode
      if ( getModeFromReceivedData(receivedData) == STOCKPILE) {
        if (prevNeighborModes[f] != STOCKPILE) {
          attack += 1;
        }
      }

      //When the Player connects to the Boss, always reduce the attack damage back to 1
      if ( getModeFromReceivedData(receivedData) == BOSSMODE) {
        if (prevNeighborModes[f] != BOSSMODE) {
          attack = 1;
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

void gameManagerMode() {

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

void gameManagerDisplay() {
  if (mode == HEAL) {
    setColor(GREEN);
  }

  if (mode == STOCKPILE) {
    setColor(BLUE);
  }
}

