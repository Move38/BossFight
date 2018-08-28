
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
  piece = BOSS;
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
   
    if (f < playerHealth) {
      setFaceColor(f, GREEN);
    }

    if (f < attack){
      setFaceColor(f + 3, BLUE);
    }

  }
}

