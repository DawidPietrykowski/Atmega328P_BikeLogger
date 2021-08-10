#pragma once

#include "Arduino.h"
#include "notes.h"
#include "const.h"

void beep_Tk(){

  int melody[] = {
    NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, NOTE_B4, NOTE_E5, 
    NOTE_E5, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, 
    NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, NOTE_D5, NOTE_FS5, 
    NOTE_FS5, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
    8, 8, 8, 4, 4, 4, 
    4, 5, 8, 8, 8, 8, 
    8, 8, 8, 4, 4, 4, 
    4, 5, 8, 8, 8, 8
  };

  for (int thisNote = 0; thisNote < 24; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.

    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.

    int noteDuration = 1000 / noteDurations[thisNote];

    tone(BUZZER_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.

    // the note's duration + 30% seems to work well:

    int pauseBetweenNotes = noteDuration * 1.30;

    delay(pauseBetweenNotes);

    // stop the tone playing:

    noTone(BUZZER_PIN);

  }
}

void beep_short(){

  tone(BUZZER_PIN, NOTE_C5);
  delay(250);
  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, NOTE_DS8);
  delay(150);
  noTone(BUZZER_PIN);
}

void beep_sad(){

  tone(BUZZER_PIN, NOTE_C5);
  delay(250);
  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, NOTE_C2);
  delay(400);
  noTone(BUZZER_PIN);
}

void beep_discharge(int n, int ms){
  for(int i = 0; i < n; i++){
    tone(BUZZER_PIN, NOTE_C5);
    delay(250);
    noTone(BUZZER_PIN);

    delay(100);

    tone(BUZZER_PIN, NOTE_C5);
    delay(250);
    noTone(BUZZER_PIN);

    delay(ms);
  }
}
