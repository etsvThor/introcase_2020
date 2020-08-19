#define THOR_SONG_LENGTH        30
#define BETWEEN_NOTE_DELAY_MS   5
#define L16                     153 // 98 BPM
#define L8                      2 * L16
#define L8D                     L8 + L16
#define L4                      2 * L8
#define L4D                     L4 + L8
#define L2                      2 * L4
#define L1                      2 * L2

const int thorSongMelody[] = {
  NOTE_F4,
  NOTE_F4,
  NOTE_F4,
  NOTE_C4,
  NOTE_A4,
  NOTE_A4,
  NOTE_A4,
  NOTE_F4,
  NOTE_F4,
  NOTE_A4,
  NOTE_C5,
  NOTE_C5,
  NOTE_AS4,
  NOTE_A4,
  NOTE_G4,
  NOTE_G4,
  NOTE_A4,
  NOTE_AS4,
  NOTE_AS4,
  NOTE_A4,
  NOTE_G4,
  NOTE_A4,
  NOTE_F4,
  NOTE_F4,
  NOTE_A4,
  NOTE_G4,
  NOTE_C4,
  NOTE_E4,
  NOTE_G4,
  NOTE_F4
};

const int thorSongDuration[] = {
  L8D,
  L16,
  L4,
  L4,
  L8D,
  L16,
  L4,
  L4,
  L8D,
  L16,
  L4D,
  L8,
  L8D,
  L16,
  L2,
  L8D,
  L16,
  L4,
  L4,
  L8D,
  L16,
  L4,
  L4,
  L8D,
  L16,
  L4D,
  L8,
  L8D,
  L16,
  L2
};