
byte bossHealth = 3;

byte attack = 1;
byte playerHealth = 3;

enum pieces {
  BOSS,
  PLAYER,
  GAMEMANAGER
};

byte piece = BOSS;

void setup() {
  // put your setup code here, to run once:
  piece = PLAYER;
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


  }

}

/*
  Game Code
*/

void bossMode() {
  if (bossHealth > 6) {
    bossHealth = 6;
  }
}

void playerMode() {

  if (playerHealth > 3) {
    playerHealth = 3;
  }

  if (attack > 3) {
    attack = 3;
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
    if (f < attack){
      setFaceColor(5-f, BLUE);
    }

  }
}

