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
#define PLAYER_START_HEALTH  1      // 
#define PLAYER_MAX_ATTACK    3      // the apprentice becomes the master
#define PLAYER_START_ATTACK  1      // we have little fighting experience

#define BOSS_PROB_HEAL       1      //  ratio for bossFight option
#define BOSS_PROB_FIGHT      1      //
#define BOSS_BUFFED_BOOST    1      // Make the boss do +1 more damage 

#define PLAYER_ATTACK_TIMEOUT  4000 // apparently we didn't want to fight in the first place
Timer attackTimer;

#define RUNE_TIMEOUT  4000 // apparently we didn't want to fight in the first place
Timer runeTimer;

/*
   ANIMATION TIMING
*/
#define HEAL_TRANSFER_DURATION       1000
#define STOCKPILE_TRANSFER_DURATION  1000
#define ATTACK_DURATION              1000
#define INJURY_DELAY                 1000
#define INJURY_DURATION              1000
#define RUNE_RETURN_DURATION         2000

Timer healAniTimer;
Timer stockpileAniTimer;
Timer attackAniTimer;
Timer injuryAniTimer;
Timer runeReturnTimer;

byte actionFace;

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

  if (buttonMultiClicked()) {
    if (buttonClickCount() == 3) {
      byte nextPiece = (piece + 1) % NUM_PIECE_TYPES;
      setPieceType( nextPiece );
    }
  }

  if (buttonLongPressed()) {
    // reset the piece
    setPieceType(piece);
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
      } else if (diceRoll > BOSS_PROB_HEAL && bossBuffed == true) {
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

        if (neighborPiece == RUNE && neighborMode == STOCKPILE) {
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

      else if (mode == ARMED) {
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
            injuryValue = attack;         // we get hit with our own arsenal
          } else if (neighborMode == ATTACKBUFF) {
            injuryValue = attack + BOSS_BUFFED_BOOST; // we get hit with our own arsenal... and then some
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
            attackAniTimer.set(ATTACK_DURATION);
            actionFace = f;
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
          actionFace = f;   // know which side we received the arms from
          // animate the arming process
          stockpileAniTimer.set(STOCKPILE_TRANSFER_DURATION);

        }
      }
      else if (mode == HEALED) {
        if (neighborPiece == RUNE && neighborMode == STANDBY) {
          health += 1;
          if (health > PLAYER_MAX_HEALTH) {
            health = PLAYER_MAX_HEALTH;
          }
          mode = STANDBY;
          actionFace = f;   // know which side we are being healed from
          // animate the healing process
          healAniTimer.set(HEAL_TRANSFER_DURATION);

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
          actionFace = f; // know which side we are healing
          healAniTimer.set(HEAL_TRANSFER_DURATION);
        }
      }
      else if (mode == STOCKPILE) {
        // return to standby if we see that we successfully armed our neighbor

        if ( (neighborPiece == PLAYER  || neighborPiece == BOSS) && neighborMode == ARMED) {
          mode = STANDBY;
          actionFace = f; // know which side we are stockpiling
          stockpileAniTimer.set(STOCKPILE_TRANSFER_DURATION);
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
  
  if (mode == STANDBY || mode == ATTACKBUFF) {
    // animate the loss of a health when attacked
  }
  
  if (mode == INJURED) {
    //setFaceColor(5, YELLOW);
    setColor(WHITE);  // flash white on hit
    // start an animation for being hit
    // we should also know from which side
  }

  if (mode == DEAD) {
    // something red
    setFaceColor(rand(6), dim(RED, rand(255)));
    // this killing the boss moment should be quite an occasion
  }

  if (mode == ATTACKBUFF) {
    // start an animation for getting buff
    // we should also know from which side
  }

  if (bossBuffed) {
    setFaceColor(0, MAGENTA);
    // continue to let everyone know we are buff, like muscle beach buff.
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

  // animate a growth in healing
  if (!healAniTimer.isExpired()) {
    // then we just got healed
    long progress = healAniTimer.getRemaining();
    long t0 = HEAL_TRANSFER_DURATION / 4;
    long t1 = 0;
    byte brightness = map_m(progress, t0, t1, 0, 255);
    if ( progress > t0 ) brightness = 0;
    if ( progress < t1 ) brightness = 255;
    setFaceColor(health - 1, dim(GREEN, brightness));
  }
  // animate a growth in attack
  if (!stockpileAniTimer.isExpired()) {
    // then we just got healed
    long progress = stockpileAniTimer.getRemaining();
    long t0 = STOCKPILE_TRANSFER_DURATION / 4;
    long t1 = 0;
    byte brightness = map_m(progress, t0, t1, 0, 255);
    if ( progress > t0 ) brightness = 0;
    if ( progress < t1 ) brightness = 255;
    setFaceColor(5 - attack + 1, dim(ORANGE, brightness));
  }

  // animate an attack to the side it is attacking (rows of 2 LEDs in a flash bulb animation)
  if (mode == STANDBY) {
    if (!attackAniTimer.isExpired()) {
      // then we are healing
      // 4 containers to remove energy from
      FOREACH_FACE(f) {
        long offset = ATTACK_DURATION / 6;
        byte dist = (actionFace + 6 - f) % 6;
        byte phase;
        switch (dist) {
          case 0: phase = 3; break;
          case 1: phase = 2; break;
          case 2: phase = 1; break;
          case 3: phase = 0; break;
          case 4: phase = 1; break;
          case 5: phase = 2; break;
        }
        long t0 = (phase * offset) + (ATTACK_DURATION - (offset * 3));
        long t1 = (phase * offset);
        long progress = attackAniTimer.getRemaining();
        byte brightness = map_m(progress, t0, t1, 255, 0);
        if (progress > t0) brightness = 0;
        if (progress < t1) brightness = 0;
        setFaceColor(f, dim(ORANGE, brightness));
      }
    }
  }

  // animate waiting in attack mode circling around with amount of attack
  if (isAttackMode(mode)) {
    // circle around with a trail for each attack level
    // 1 with trail for attack 1
    // 2 with trails on opposite sides for attack 2
    // 3 with trails for attack 3
    long rotation = (millis() / 3) % 360;
    byte head = rotation / 60;
    byte brightness;

    FOREACH_FACE(f) {

      byte distFromHead = (6 + head - f) % 6; // returns # of positions away from the head
      long degFromHead = (360 + rotation - 60 * f) % 360; // returns degrees away from the head

      if (attack == 2) {
        if (distFromHead >= 3) {
          distFromHead -= 3;
          degFromHead -= 180;
        }
      }
      else if (attack == 3) {
        while (distFromHead >= 2) {
          distFromHead -= 2;
          degFromHead -= 120;
        }
      }
      if (distFromHead < (4 - attack)) {
        brightness = map_m(degFromHead, 0, 60 * (4 - attack), 255, 0); // scale the brightness to 8 bits and dimmer based on distance from head
      } else {
        brightness = 0; // don't show past the tail of the snake
      }
      setFaceColor(f, dim(ORANGE, brightness));
    }
  }

  // animate a defeated player (maybe looks like a boxer seeing stars...)
  if (mode == DEAD) {
    // flash blue and green
    if ((millis() / 500) % 2 == 0) {
      setColor(dim(ORANGE, 127));
    }
    else {
      setColor(dim(GREEN, 127));
    }
  }
}

void runeDisplay() {
  if (mode == HEAL) {
    // animate to show we are ready to heal
    setColor(GREEN);
  }
  else if (mode == STOCKPILE) {
    // animate to show we are going to stockpile
    setColor(ORANGE);
  }
  else {  // STANDBY

    if (!runeReturnTimer.isExpired()) {
      // fade up to white from off
      long t0 = RUNE_RETURN_DURATION / 2;
      long t1 = 0;
      long progress = runeReturnTimer.getRemaining();
      byte brightness = map_m(progress, t0, t1, 0, 127);
      if (progress > t0) brightness = 0;  // hold dark for a bit then return
      setColor(dim(WHITE, brightness));
    }
    else {
      setColor(dim(WHITE, 127));
    }

    // check if we just stockpiled
    // start an animation for stockpiling
    // we should also know to which side
    if (!stockpileAniTimer.isExpired()) {
      // then we are healing
      // 4 containers to remove energy from
      FOREACH_FACE(f) {
        long offset = STOCKPILE_TRANSFER_DURATION  / 6;
        byte dist = (actionFace + 6 - f) % 6;
        byte phase;
        switch (dist) {
          case 0: phase = 0; break;
          case 1: phase = 1; break;
          case 2: phase = 2; break;
          case 3: phase = 3; break;
          case 4: phase = 2; break;
          case 5: phase = 1; break;
        }
        long t0 = (phase * offset) + (STOCKPILE_TRANSFER_DURATION - (offset * 3));
        long t1 = (phase * offset);
        long progress = stockpileAniTimer.getRemaining();
        byte brightness = map_m(progress, t0, t1, 255, 0);
        if (progress > t0) brightness = 255;
        if (progress < t1) brightness = 0;
        setFaceColor(f, dim(ORANGE, brightness));
      }

      runeReturnTimer.set(RUNE_RETURN_DURATION);  // keep this loaded until we are done
    }

    // check if we just healed
    // start an animation for healing
    // we should also know to which side
    // animate a growth in healing
    if (!healAniTimer.isExpired()) {
      // then we are healing
      // 4 containers to remove energy from
      FOREACH_FACE(f) {
        long offset = HEAL_TRANSFER_DURATION / 6;
        byte dist = (actionFace + 6 - f) % 6;
        byte phase;
        switch (dist) {
          case 0: phase = 0; break;
          case 1: phase = 1; break;
          case 2: phase = 2; break;
          case 3: phase = 3; break;
          case 4: phase = 2; break;
          case 5: phase = 1; break;
        }
        long t0 = (phase * offset) + (HEAL_TRANSFER_DURATION - (offset * 3));
        long t1 = (phase * offset);
        long progress = healAniTimer.getRemaining();
        byte brightness = map_m(progress, t0, t1, 255, 0);
        if (progress > t0) brightness = 255;
        if (progress < t1) brightness = 0;
        setFaceColor(f, dim(GREEN, brightness));
      }

      runeReturnTimer.set(RUNE_RETURN_DURATION);  // keep this loaded until we are done
    }
  }
}

/*
   -------------------------------------------------------------------------------------
                                 HELPER FUNCTIONS
   -------------------------------------------------------------------------------------
*/

/*
  This map() functuion is now in Arduino.h in /dev
  It is replicated here so this skect can compile on older API commits
*/

long map_m(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
   Sin in degrees ( standard sin() takes radians )
*/

float sin_d( uint16_t degrees ) {

  return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}

