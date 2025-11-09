#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

// Animation
byte bottom1[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11011
};

byte bottom2[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11000,
  B11011
};

byte bottom3[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00011,
  B00011,
  B11011,
  B11011
};

byte bottom4[8] = {
  B00000,
  B00000,
  B00000,
  B11000,
  B11000,
  B11000,
  B11011,
  B11011
};

byte bottom5[8] = {
  B00011,
  B00011,
  B00011,
  B00011,
  B11011,
  B11011,
  B11011,
  B11011
};

byte bottom6[8] = {
  B11000,
  B11000,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011
};

byte play[8] = {
  B00000,
  B11000,
  B11110,
  B11111,
  B11111,
  B11110,
  B11000,
  B00000
};

byte pause[8] = {
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B00000
};

const int PUSH_SHORT = 100;
const int PUSH_LONG = 32000;

const int COOLDOWN = 2400;

const int ANIMATION_RATE = 4000;

const int BACKLIGHT_PIN = 2;
const int B_LEFT = A1;
const int B_CENTER = A4;
const int B_RIGHT = A5;

const String menuText[11] = { " [Music Player] ", "", "", "     [Menu]     ", "    [Volume]    ", "   [Playback]   ", "  [Backlight]   ", "", "   [Language]   ", "      [EQ]      ", "  [Visualizer]  " };
const String homeMenu[2] = { "Listen", "Settings" };
const String menu[9] = { "Back to Title", "Select Folder", "Select Track", "Exit Menu", "Volume", "Playback Mode", "Backlight", "EQ", "Visualizer" };
//const String menu[6] = { "Volume", "Playback mode", "Backlight", "Language", "Back to title", "Exit menu" };
const String playbackMenu[3] = { "Normal", "Repeat", "Loop" };
const String toggleMenu[2] = { "OFF", "ON" };
const String EQMenu[6] = { "Normal", "Pop", "Rock", "Jazz", "Classic", "Bass" };
const String languageMenu[2] = { "English", "ﾆﾎﾝｺﾞ" };

bool left_press = false;
bool center_press = false;
bool right_press = false;

int left_count = 0;
int center_count = 0;
int right_count = 0;

int center_cooldown = 0;

int animation_count = 0;

int current_folder = 0;
int current_track = 0;
int old_folder = 0;
int old_track = 0;
int menu_item = 0;
int mode = 0;  // 0: Home, 1: Folder, 2: Song, 3: Context menu, 4: Volume, 5: Playback menu, 6: Backlight menu, 7: Now Playing, 8: Language menu, 9: EQ, 10: Animation

bool language = false;
int eq = 0;
int backlight = 0;
int volume = 0;
int playback_mode = 1;  // 0: Normal, 1: Repeat, 2: Loop

bool can_animate = true;

int oldMenuItem = 0;
int oldMode = 0;

int back_menu_item = 0;
int back_mode = 0;

int playing_folder = 0;
int old_playing_folder = 0;
int playing_song = 0;
int old_playing_song = 0;

int error_folder = 0;
int error_song = 0;

bool is_playing = false;
bool old_playing = false;

bool is_pressed = false;

bool first_frame = false;

int folder_counts = 0;
int file_counts = 0;

int error = 0;  // 0: No error, 1: File N/A, 2: File index out of range, 3: Busy, 4: Time out, 5: Card inserted, 6: Card removed
bool was_inserted = false;

LiquidCrystal lcd(6, 5, 8, 7, 4, 3);

SoftwareSerial mySerial(10, 11);
DFRobotDFPlayerMini dfPlayer;

void (*restart)(void) = 0;

void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);

  lcd.createChar(0, bottom1);
  lcd.createChar(1, bottom2);
  lcd.createChar(2, bottom3);
  lcd.createChar(3, bottom4);
  lcd.createChar(4, bottom5);
  lcd.createChar(5, bottom6);
  lcd.createChar(6, play);
  lcd.createChar(7, pause);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Starting...   ");

  pinMode(BACKLIGHT_PIN, OUTPUT);

  pinMode(B_LEFT, INPUT_PULLUP);
  pinMode(B_CENTER, INPUT_PULLUP);
  pinMode(B_RIGHT, INPUT_PULLUP);

  mySerial.begin(9600);
  Serial.begin(9600);

  if (!dfPlayer.begin(mySerial)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unable to begin.");
    lcd.setCursor(0, 1);
    lcd.print("Press to restart");
    do { delay(1); } while (!(digitalRead(B_LEFT) == LOW || digitalRead(B_CENTER) == LOW || digitalRead(B_RIGHT) == LOW));
    do { delay(1); } while (!(digitalRead(B_LEFT) == HIGH && digitalRead(B_CENTER) == HIGH && digitalRead(B_RIGHT) == HIGH));
    restart();
  }

  Serial.println(F("DFPlayer Mini online."));

  dfPlayer.volume(5);
  volume = 5;

  analogWrite(BACKLIGHT_PIN, 0);

  drawMenu();
}

void loop() {
  // put your main code here, to run repeatedly:
  int value = digitalRead(B_LEFT);
  if (value == LOW) {
    if (left_count <= PUSH_SHORT) left_count++;
  } else {
    if (mode == 7) {
      if (left_count != 0 && left_count < COOLDOWN) left_count++;
      else if (left_count >= COOLDOWN) left_count = 0;
    } else {
      left_count = 0;
    }
  }
  if (left_count == PUSH_SHORT) {
    error = 0;
    if (menu_item > 0) {
      menu_item--;
      is_pressed = true;
      if (mode == 6) {
        backlight = menu_item;
        analogWrite(BACKLIGHT_PIN, 128 * menu_item);
      }
      if (mode == 8) {
        if (menu_item == 0) {
          // lcd.kanaOff();
          language = false;
        } else {
          // lcd.kanaOn();
          language = true;
        }
      }
      if (mode == 9) {
        dfPlayer.EQ(menu_item);
        eq = menu_item;
      }
    } else if (mode == 4) {
      if (volume > 0) {
        volume--;
        dfPlayer.volume(volume);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(menuText[4]);
        lcd.setCursor((16 - ((String)volume).length()) / 2, 1);
        lcd.print(volume);
      }
    } else if (mode == 7) {
      if (current_track > 1) {
        current_track--;
        old_playing_song = playing_song;
        playing_song = current_track;
        old_playing_folder = playing_folder;
        playing_folder = current_folder;
        is_playing = true;
        if (current_track > 0 && current_folder > 0) {
          dfPlayer.playFolder(current_folder, current_track);
        } else if (current_folder == 0) {
          dfPlayer.playMp3Folder(current_track);
        }
      }
    }
  }

  value = digitalRead(B_CENTER);
  if (value == LOW) {
    if (center_count != PUSH_LONG) {
      center_count++;
    } else if (center_count == PUSH_LONG) {
      if (mode != 3 && mode != 4 && mode != 5 && mode != 6 && mode != 8) {
        back_mode = mode;
        mode = 3;
        back_menu_item = menu_item;
        menu_item = 4;
      }
    }
  } else if (center_cooldown == 0) {
    if (center_count < PUSH_LONG && center_count >= PUSH_SHORT) {
      switch (mode) {
        case 0:
          switch (menu_item) {
            case 0:
              back_mode = mode;
              mode = 1;
              back_menu_item = menu_item;
              menu_item = 0;
              folder_counts = 100;
              // folder_counts = dfPlayer.readFolderCounts() + 1;
              break;
            case 1:
              back_mode = mode;
              mode = 3;
              back_menu_item = menu_item;
              menu_item = 4;
              break;
            default: break;
          }
          break;
        case 1:
          mode = 2;
          back_mode = mode;
          back_menu_item = menu_item;
          menu_item = 0;
          file_counts = 255;
          // if (current_folder > 0) {
          //   file_counts = dfPlayer.readFileCountsInFolder(current_folder);
          // } else {
          //   file_counts = dfPlayer.readFileCounts();
          // }
          break;
        case 2:
          mode = 7;
          menu_item = 0;
          is_playing = true;
          old_playing_song = playing_song;
          playing_song = current_track;
          old_playing_folder = playing_folder;
          playing_folder = current_folder;
          if (current_folder == 0) {
            dfPlayer.playMp3Folder(current_track);
          } else {
            dfPlayer.playFolder(current_folder, current_track);
          }
          break;
        case 3:
          switch (menu_item) {
            case 0:
              mode = 0;
              menu_item = 0;
              is_playing = false;
              dfPlayer.pause();
              break;
            case 1:
              is_playing = false;
              dfPlayer.pause();
              back_menu_item = menu_item;
              if (back_mode == 7 || back_mode == 2) {
                menu_item = current_folder;
              } else {
                menu_item = 0;
              }
              back_mode = mode;
              mode = 1;
              folder_counts = 100;
              // folder_counts = dfPlayer.readFolderCounts() + 1;
              break;
            case 2:
              is_playing = false;
              dfPlayer.pause();
              back_menu_item = menu_item;
              if (back_mode == 7) {
                menu_item = current_track - 1;
              } else {
                menu_item = 0;
              }
              back_mode = mode;
              mode = 2;
              file_counts = 255;
              break;
            case 3:
              mode = back_mode;
              menu_item = back_menu_item;
              break;
            case 4:
              mode = 4;
              menu_item = 0;
              break;
            case 5:
              mode = 5;
              menu_item = playback_mode;
              break;
            case 6:
              mode = 6;
              menu_item = backlight;
              break;
            case 7:
              mode = 9;
              menu_item = dfPlayer.readEQ();
              break;
            case 8:
              mode = 10;
              if (can_animate) {
                menu_item = 1;
              } else {
                menu_item = 0;
              }
              break;
            default: break;
          }
          break;
        case 4:
          mode = back_mode;
          menu_item = back_menu_item;
          break;
        case 5:
          switch (menu_item) {
            case 0:
              playback_mode = 0;
              break;
            case 1:
              playback_mode = 1;
              break;
            case 2:
              playback_mode = 2;
              break;
            default:
              break;
          }
          mode = back_mode;
          menu_item = back_menu_item;
          break;
        case 6:
          mode = back_mode;
          menu_item = back_menu_item;
          break;
        case 7:
          if (is_playing) {
            is_playing = false;
            dfPlayer.pause();
          } else {
            if (error == 1) {
              error_song = current_track;
              error_folder = current_folder;
              current_track = old_playing_song;
              current_folder = old_playing_folder;
              error = 0;
            }
            if (playing_song == current_track && playing_folder == current_folder) {
              if (current_track != error_song && current_folder != error_folder) {
                error = 0;
                is_playing = true;
                dfPlayer.start();
              } else {
                error = 1;
              }
            }
            //dfPlayer.playFolder(current_folder, current_track);
          }
          break;
        case 8:
          mode = back_mode;
          menu_item = back_menu_item;
        case 9:
          mode = back_mode;
          menu_item = back_menu_item;
        case 10:
          if (menu_item == 0) {
            can_animate = false;
          } else {
            can_animate = true;
          }
          mode = back_mode;
          menu_item = back_menu_item;
        default: break;
      }
    }
    center_cooldown = COOLDOWN;
    center_count = 0;
  }

  if (center_cooldown > 0) center_cooldown--;

  value = digitalRead(B_RIGHT);
  if (value == LOW) {
    if (right_count <= PUSH_SHORT) right_count++;
  } else {
    if (mode == 7) {
      if (right_count != 0 && right_count < COOLDOWN) right_count++;
      else if (right_count >= COOLDOWN) right_count = 0;
    } else {
      right_count = 0;
    }
  }
  if (right_count == PUSH_SHORT) {
    error = 0;
    if (menu_item < menuLength() - 1) {
      menu_item++;
      is_pressed = true;
      if (mode == 6) {
        backlight = menu_item;
        analogWrite(BACKLIGHT_PIN, 128 * menu_item);
      }
      if (mode == 8) {
        if (menu_item == 0) {
          // lcd.kanaOff();
          language = false;
        } else {
          // lcd.kanaOn();
          language = true;
        }
      }
      if (mode == 9) {
        dfPlayer.EQ(menu_item);
        eq = menu_item;
      }
    } else if (mode == 4) {
      if (volume < 30) {
        volume++;
        dfPlayer.volume(volume);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(menuText[4]);
        lcd.setCursor((16 - ((String)volume).length()) / 2, 1);
        lcd.print(volume);
      }
    } else if (mode == 7) {
      current_track++;
      old_playing_song = playing_song;
      playing_song = current_track;
      old_playing_folder = playing_folder;
      playing_folder = current_folder;
      is_playing = true;
      if (current_track < file_counts && current_folder > 0) {
        dfPlayer.playFolder(current_folder, current_track);
      } else if (current_folder == 0) {
        dfPlayer.playMp3Folder(current_track);
      }
    }
  }

  if (mode != oldMode || menu_item != oldMenuItem) {
    if (mode == 1) {  // Folder
      if (folder_counts > 0) {
        current_folder = menu_item;
        //file_counts = dfPlayer.readFileCountsInFolder(current_folder);
        String string = "Folder " + (String)current_folder;
        if (menu_item == 0) {
          string = "MP3 Folder";
        }
        int space = (16 - string.length()) / 2;
        String string2 = "Folder " + (String)(current_folder + 1);
        int space2 = (16 - string2.length()) / 2;
        lcd.clear();
        lcd.setCursor(space, 0);
        lcd.print(string);
        lcd.setCursor(0, 0);
        lcd.print(">");
        lcd.setCursor(15, 0);
        lcd.print("<");
        if (current_folder < folder_counts - 1) {
          lcd.setCursor(space2, 1);
          lcd.print(string2);
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   No folders.  ");
      }
    } else if (mode == 2) {  // Song
      if (file_counts > 0) {
        current_track = menu_item + 1;
        String string = (String)current_folder + "/Track " + (String)current_track;
        int space = (16 - string.length()) / 2;
        String string2 = (String)current_folder + "/Track " + (String)(current_track + 1);
        int space2 = (16 - string2.length()) / 2;
        lcd.clear();
        lcd.setCursor(space, 0);
        lcd.print(string);
        lcd.setCursor(0, 0);
        lcd.print(">");
        lcd.setCursor(15, 0);
        lcd.print("<");
        if (current_track < file_counts) {
          lcd.setCursor(space2, 1);
          lcd.print(string2);
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   No tracks.   ");
      }
    } else if (mode == 4) {  // Volume
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(menuText[4]);
      lcd.setCursor((16 - ((String)volume).length()) / 2, 1);
      lcd.print(volume);
    } else if (mode != 7) {  // Menu
      drawMenu();
    }
  }

  if (mode == 7 && (old_folder != current_folder || old_track != current_track || old_playing != is_playing || oldMode != mode)) {
    if (can_animate) {
      if (is_playing) {
        if (first_frame) {
          visualizer();
          first_frame = false;
        }
      } else {
        visualizer();
        first_frame = true;
      }
    } else {
      lcd.clear();
      lcd.print("Fld " + (String)current_folder);
      lcd.setCursor(0, 1);
      lcd.print("Trk " + (String)current_track);
      lcd.setCursor(8, 0);
      lcd.print(playbackMenu[playback_mode]);
      lcd.setCursor(8, 1);
      lcd.print(EQMenu[eq]);
      lcd.setCursor(7, 0);
      lcd.print("|");
      lcd.setCursor(7, 1);
      lcd.print("|");
      lcd.setCursor(15, 0);
      if (error == 0) {
        if (is_playing) {
          lcd.write(byte(6));
        } else {
          first_frame = true;
          lcd.write(byte(7));
        }
      } else {
        printError();
      }
    }
    old_folder = current_folder;
    old_track = current_track;
    old_playing = is_playing;
  }

  if (mode == 7 && can_animate && is_playing) {
    if (animation_count == ANIMATION_RATE) {
      visualizer();
      animation_count = 0;
    } else {
      animation_count++;
    }
  }

  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read());  //Print the detail message from DFPlayer to handle different errors and states.
  }

  oldMode = mode;
  oldMenuItem = menu_item;
}

int menuLength() {
  switch (mode) {
    case 0: return 2; break;
    case 1: return folder_counts; break;
    case 2: return file_counts; break;
    case 3: return 9; break;
    case 4: return 0; break;
    case 5: return 3; break;
    case 6: return 2; break;
    case 7: return 0; break;
    case 8: return 2; break;
    case 9: return 6; break;
    case 10: return 2; break;
    default: return 0; break;
  }
}

void drawMenu() {
  String string;
  int spacer;
  if (mode == 0) {
    string = homeMenu[menu_item];
  } else if (mode == 3) {
    string = menu[menu_item];
  } else if (mode == 5) {
    string = playbackMenu[menu_item];
  } else if (mode == 6) {
    string = toggleMenu[menu_item];
  } else if (mode == 8) {
    string = languageMenu[menu_item];
  } else if (mode == 9) {
    string = EQMenu[menu_item];
  } else if (mode == 10) {
    string = toggleMenu[menu_item];
  } else {
    return;
  }
  spacer = (16 - string.length()) / 2;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(menuText[mode]);
  lcd.setCursor(spacer, 1);
  lcd.print(string);
  if (menu_item > 0) {
    lcd.setCursor(0, 1);
    lcd.print("<");
  }
  if (menu_item < menuLength() - 1) {
    lcd.setCursor(15, 1);
    lcd.print(">");
  }
}

void visualizer() {
  lcd.clear();
  String trackNo = (String)current_folder + "/Track " + (String)current_track;
  lcd.setCursor(0, 0);
  lcd.print(trackNo);
  lcd.setCursor(15, 0);
  lcd.write(byte(is_playing ? 6 : 7));
  if (error == 0) {
    if (is_playing) {
      for (int i = 0; i < 16; i++) {
        lcd.setCursor(i, 1);
        lcd.write(byte(random(0, 6)));
      }
    } else {
      for (int i = 0; i < 16; i++) {
        lcd.setCursor(i, 1);
        lcd.write(byte(0));
      }
    }
  } else {
    printError();
  }
}

void printError() {
  if (error != 0) {
    lcd.setCursor(0, 1);
    switch (error) {
      case 1:
        lcd.print("   [File N/A]   ");
        break;
      case 2:
        lcd.print(" [Out of Bound] ");
        break;
      case 3:
        lcd.print("[Card not found]");
        break;
      case 4:
        lcd.print("   [Time Out]   ");
        break;
      default:
        break;
    }
  }
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      // if (is_playing) {
      //   is_playing = false;
      //   dfPlayer.pause();
      // }
      // if (error != 6) {
      //   error = 4;
      //   timeout();
      // }
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      was_inserted = true;
      Serial.println(was_inserted ? "TRUE" : "FALSE");
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      if (is_playing) {
        is_playing = false;
        dfPlayer.pause();
      }
      if (error != 6) {
        was_inserted = false;
        cardNotFound();
      }
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number: "));
      Serial.print(value);
      Serial.println(F(" | Play Finished!"));
      if (playback_mode == 0) {
        is_playing = false;
      } else if (playback_mode == 1) {
        current_track++;
        old_playing_song = playing_song;
        playing_song = current_track;
        old_playing_folder = playing_folder;
        playing_folder = current_folder;
        if (current_folder == 0) {
          dfPlayer.playMp3Folder(current_track);
        } else {
          dfPlayer.playFolder(current_folder, current_track);
        }
      } else if (playback_mode == 2) {
        old_playing_song = playing_song;
        playing_song = current_track;
        old_playing_folder = playing_folder;
        playing_folder = current_folder;
        if (current_folder == 0) {
          dfPlayer.playMp3Folder(current_track);
        } else {
          dfPlayer.playFolder(current_folder, current_track);
        }
      }
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError: "));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          if (is_playing) {
            is_playing = false;
            dfPlayer.pause();
          }
          error = 2;
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          if (playback_mode == 1 && current_track != 1) {
            current_track = 1;
            if (is_playing) {
              old_playing_song = playing_song;
              playing_song = current_track;
              old_playing_folder = playing_folder;
              playing_folder = current_folder;
              if (current_folder == 0) {
                dfPlayer.playMp3Folder(current_track);
              } else {
                dfPlayer.playFolder(current_folder, current_track);
              }
            }
          } else {
            error_folder = current_folder;
            error_song = current_track;
            playing_song = old_playing_song;
            playing_folder = old_playing_folder;
            if (is_playing) {
              is_playing = false;
              dfPlayer.pause();
            }
            error = 1;
          }
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void timeout() {
  lcd.clear();
  lcd.print("[Time out error]");
  lcd.setCursor(0, 1);
  lcd.print("Press any button");
  do { delay(1); } while (digitalRead(B_LEFT) == HIGH && digitalRead(B_CENTER) == HIGH && digitalRead(B_RIGHT) == HIGH);
  do { delay(1); } while (digitalRead(B_LEFT) == LOW || digitalRead(B_CENTER) == LOW || digitalRead(B_RIGHT) == LOW);
  delay(500);
  error = 0;
  mode = 0;
  menu_item = 0;
}

void cardNotFound() {
  error = 6;
  lcd.clear();
  lcd.print(" [Card removed] ");
  lcd.setCursor(0, 1);
  lcd.print("  Insert card   ");
  while (!was_inserted) {
    if (dfPlayer.available()) {
      printDetail(dfPlayer.readType(), dfPlayer.read());
    }
  }
  error = 0;
  mode = 0;
  menu_item = 0;
  drawMenu();
}
