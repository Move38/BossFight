
byte bossHealth = 3;

byte attack = 1;
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
  HEAL
};

byte mode = STANDBY;

byte prevNeighborModes[6];

//byte modeReceived = STANDBY;

void setup() {
  // put your setup code here, to run once:
  piece = BOSS;

  if (piece == GAMEMANAGER) {
    mode = HEAL;
  }

}

void loop() {
  // put your main code here, to run repeatedly:

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

  setValueSentOnAllFaces(mode);

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      prevNeighborModes[f] = getLastValueReceivedOnFace(f);
    } else {
      prevNeighborModes[f] = STANDBY;
    }
  }
}

/*
  Game Code
*/

void bossMode() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getLastValueReceivedOnFace(f) == HEAL) {
        if (prevNeighborModes[f] != HEAL) {
          bossHealth += 1;
        }
      }
    }

    if (bossHealth > 6) {
      bossHealth = 6;
    }

  }
}

void playerMode() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getLastValueReceivedOnFace(f) == HEAL) {
        if (prevNeighborModes[f] != HEAL) {
          playerHealth += 1;
        }
      }

      if ( getLastValueReceivedOnFace(f) == STOCKPILE) {
        if (prevNeighborModes[f] != STOCKPILE) {
          attack += 1;
        }
      }
    }
  }

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
  FOREACH_FACE(f) {
    if (f < bossHealth) {
      setFaceColor(f, RED);
    }
  }
}

void playerDisplay() {
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

