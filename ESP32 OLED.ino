#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Definisi Pin Hardware ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C 

// Pin Tombol
#define UP_PIN 13       
#define DOWN_PIN 12     
#define OK_PIN 14       

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- State Management ---
enum AppState {
    HOME_SCREEN,         
    MODE_SELECT_MENU,    
    FOCUS_MENU,          
    SUBJECT_SELECT,      
    TIMER_SET_MINUTES,   
    TIMER_SET_SECONDS,   
    TIMER_RUNNING,       
    TIMER_PAUSED,        
    TIME_UP_PR_DONE,     
    TIME_UP_ADD_TIME,    
};

AppState currentState = HOME_SCREEN;
int menuSelection = 0; 
int subjectSelection = 0; 
const char* subjects[] = {"Inggris", "PKN", "MTK"};

// Timer Variables
unsigned long targetDuration = 0; 
unsigned long timerStartTime = 0; 
unsigned long timeRemaining = 0; 
bool isTimerActive = false;

// Editable Timer Variables
int setMinutes = 0; 
int setSeconds = 0; 

// Simulate Time (tanpa RTC)
unsigned long timeOffset = 0;

// --- Setup ---
void setup() {
    Serial.begin(115199);
    
    Wire.begin(); 
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 alokasi gagal"));
        for (;;); 
    }

    pinMode(UP_PIN, INPUT_PULLUP);
    pinMode(DOWN_PIN, INPUT_PULLUP);
    pinMode(OK_PIN, INPUT_PULLUP);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("MemoCase Ready!");
    display.display();
    delay(1500);

    timeOffset = millis(); 
}

// --- Fungsi Tombol ---
bool isButtonPressed(int pin) {
    return (digitalRead(pin) == LOW);
}

void handleInput() {
    
    // Tombol UP (Pin 13)
    if (isButtonPressed(UP_PIN)) {
        if (currentState == HOME_SCREEN || currentState == MODE_SELECT_MENU || currentState == FOCUS_MENU) {
            menuSelection = (menuSelection == 0) ? 2 : menuSelection - 1; 
        } else if (currentState == SUBJECT_SELECT) {
            subjectSelection = (subjectSelection == 0) ? 2 : subjectSelection - 1; 
        } else if (currentState == TIMER_SET_MINUTES) {
            setMinutes = (setMinutes < 59) ? setMinutes + 1 : 0; 
        } else if (currentState == TIMER_SET_SECONDS) {
            setSeconds = (setSeconds < 59) ? setSeconds + 1 : 0; 
        } else if (currentState == TIME_UP_PR_DONE || currentState == TIME_UP_ADD_TIME) {
            menuSelection = (menuSelection == 0) ? 1 : 0; 
        }
        delay(200);
    }
    
    // Tombol DOWN (Pin 12)
    if (isButtonPressed(DOWN_PIN)) {
        if (currentState == HOME_SCREEN || currentState == MODE_SELECT_MENU || currentState == FOCUS_MENU) {
            menuSelection = (menuSelection == 2) ? 0 : menuSelection + 1; 
        } else if (currentState == SUBJECT_SELECT) {
            subjectSelection = (subjectSelection == 2) ? 0 : subjectSelection + 1; 
        } else if (currentState == TIMER_SET_MINUTES) {
            setMinutes = (setMinutes == 0) ? 59 : setMinutes - 1; 
        } else if (currentState == TIMER_SET_SECONDS) {
            setSeconds = (setSeconds == 0) ? 59 : setSeconds - 1; 
        } else if (currentState == TIME_UP_PR_DONE || currentState == TIME_UP_ADD_TIME) {
            menuSelection = (menuSelection == 1) ? 0 : 1; 
        }
        // Down sebagai STOP TOTAL (Saat Jeda/Pause)
        if (currentState == TIMER_PAUSED) {
            isTimerActive = false;
            currentState = HOME_SCREEN;
            timeRemaining = 0; 
            targetDuration = 0; 
            menuSelection = 0; 
        }
        delay(200);
    }

    // Tombol OK (Pin 14)
    if (isButtonPressed(OK_PIN)) {
        if (currentState == HOME_SCREEN) {
            if (menuSelection == 0) { 
                currentState = MODE_SELECT_MENU;
            } 
            menuSelection = 0; 
        } else if (currentState == MODE_SELECT_MENU) {
            if (menuSelection == 0) { 
                currentState = FOCUS_MENU;
            } else if (menuSelection == 1) { 
                // PILIH MODE NORMAL -> KEMBALI KE HOME (Fitur Kosong)
                currentState = HOME_SCREEN; 
            } else if (menuSelection == 2) { 
                currentState = HOME_SCREEN; 
            }
            menuSelection = 0; 
        } else if (currentState == FOCUS_MENU) {
            if (menuSelection == 0) { 
                currentState = SUBJECT_SELECT;
            } else if (menuSelection == 1) { 
                currentState = TIMER_SET_MINUTES;
                setMinutes = 0; setSeconds = 0; 
            } else if (menuSelection == 2) { 
                currentState = MODE_SELECT_MENU; 
            }
            menuSelection = 0; 
        } else if (currentState == SUBJECT_SELECT) {
            if (subjectSelection >= 0 && subjectSelection < 3) {
                currentState = TIMER_SET_MINUTES; 
                setMinutes = 0; setSeconds = 0; 
            }
        } else if (currentState == TIMER_SET_MINUTES) {
            currentState = TIMER_SET_SECONDS;
        } else if (currentState == TIMER_SET_SECONDS) {
            // Logika Start Timer (Hanya untuk Mode Fokus)
            targetDuration = (unsigned long)(setMinutes * 60 + setSeconds) * 1000UL;
            
            if (targetDuration == 0) {
                // 00:00 dipilih, langsung ke Waktu Habis
                currentState = TIME_UP_PR_DONE;
            } else {
                // START TIMER 
                timerStartTime = millis();
                timeRemaining = targetDuration; 
                isTimerActive = true;
                currentState = TIMER_RUNNING; 
            }
        } else if (currentState == TIMER_PAUSED) {
            // Lanjutkan Timer
            targetDuration = timeRemaining;
            timerStartTime = millis(); 
            isTimerActive = true;
            currentState = TIMER_RUNNING; 
        } else if (currentState == TIMER_RUNNING) {
             // Jeda Timer (OK)
             unsigned long timeElapsed = millis() - timerStartTime;
             if (timeElapsed < targetDuration) {
                 timeRemaining = targetDuration - timeElapsed;
             } else {
                 timeRemaining = 0; 
             }
             isTimerActive = false;
             currentState = TIMER_PAUSED;
        } else if (currentState == TIME_UP_PR_DONE) {
            if (menuSelection == 0) { 
                currentState = FOCUS_MENU; 
                menuSelection = 0; 
            } else { 
                currentState = TIME_UP_ADD_TIME; 
                menuSelection = 0; 
            }
        } else if (currentState == TIME_UP_ADD_TIME) {
            if (menuSelection == 0) { 
                currentState = TIMER_SET_MINUTES; 
                setMinutes = 0; setSeconds = 0;
            } else { 
                currentState = FOCUS_MENU; 
                menuSelection = 0; 
            }
        }
        delay(250);
    }
}

// --- Fungsi Display ---
void displayMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    unsigned long timeElapsedSim = millis() - timeOffset;
    int hours = (timeElapsedSim / 1000 / 3600) % 24;
    int minutes = (timeElapsedSim / 1000 / 60) % 60;
    int seconds = (timeElapsedSim / 1000) % 60;
    
    // Konstanta posisi timer
    const int TIMER_X = 10; 
    const int TIMER_Y = 30;

    switch (currentState) {
        case HOME_SCREEN:
            display.setTextSize(2);
            display.printf("%02d:%02d:%02d\n", hours, minutes, seconds); 
            display.setTextSize(1);
            display.println("12/12/2025"); 
            display.setCursor(0, 25);
            display.print(menuSelection == 0 ? "> " : "  ");
            display.println("Pilih Mode");
            display.print(menuSelection == 1 ? "> " : "  ");
            display.println("WiFi Setup"); // Dipindahkan dari Mode Normal ke sini
            break;

        case MODE_SELECT_MENU:
            display.println("--- Pilih Mode ---");
            display.print(menuSelection == 0 ? "> " : "  ");
            display.println("Mode Fokus");
            display.print(menuSelection == 1 ? "> " : "  ");
            display.println("Mode Normal"); // Perubahan di sini
            display.print(menuSelection == 2 ? "> " : "  ");
            display.println("<< Kembali"); 
            break;

        case FOCUS_MENU:
            display.println("--- Mode Fokus ---");
            display.print(menuSelection == 0 ? "> " : "  ");
            display.println("Pilih PR");
            display.print(menuSelection == 1 ? "> " : "  ");
            display.println("Timer Saja");
            display.print(menuSelection == 2 ? "> " : "  ");
            display.println("<< Kembali"); 
            break;

        case SUBJECT_SELECT:
            display.println("--- Pilih PR (OK:Lanjut) ---");
            for (int i = 0; i < 3; i++) {
                display.print(subjectSelection == i ? "> " : "  ");
                display.println(subjects[i]);
            }
            break;

        case TIMER_SET_MINUTES: { 
            display.println("--- Set Target: Menit (OK:Lanjut) ---");
            display.setTextSize(3);
            display.setCursor(TIMER_X, TIMER_Y); 
            display.printf(">%02d", setMinutes);
            display.print(":");
            display.printf("%02d", setSeconds);
            break;
        }

        case TIMER_SET_SECONDS: { 
            display.println("--- Set Target: Detik (OK:Mulai) ---"); 
            display.setTextSize(3);
            display.setCursor(TIMER_X, TIMER_Y); 
            display.printf("%02d", setMinutes);
            display.print(":>");
            display.printf("%02d", setSeconds);
            break;
        }

        case TIMER_RUNNING: { 
            unsigned long currentMillis = millis();
            unsigned long timeElapsed = currentMillis - timerStartTime; 

            // Waktu Habis (Deteksi Anti-Overflow)
            if (isTimerActive && timeElapsed >= targetDuration) { 
                timeRemaining = 0;
                isTimerActive = false; 
                currentState = TIME_UP_PR_DONE; 
                menuSelection = 0; 
                break; // Keluar agar display langsung ke TIME_UP_PR_DONE
            } 
            
            // Hitungan Mundur
            timeRemaining = targetDuration - timeElapsed;
            
            int currentMinutes = timeRemaining / 1000 / 60;
            int currentSeconds = (timeRemaining / 1000) % 60;

            display.println("--- TIMER AKTIF (OK: Jeda) ---");
            
            display.print("Fokus: ");
            display.println(subjects[subjectSelection]); 
            
            display.setTextSize(3);
            display.setCursor(TIMER_X, TIMER_Y); 
            display.printf("%02d:%02d", currentMinutes, currentSeconds); 
            break;
        }

        case TIMER_PAUSED: { 
            int p_minutes = timeRemaining / 1000 / 60;
            int p_seconds = (timeRemaining / 1000) % 60;

            display.println("--- JEDA (OK:Lanjut) ---");
            display.setCursor(0, 15); 
            display.println("DOWN untuk kembali ke Home.");
            
            display.setTextSize(3);
            display.setCursor(TIMER_X, TIMER_Y); 
            display.printf("%02d:%02d", p_minutes, p_seconds); 
            break;
        }
            
        case TIME_UP_PR_DONE: 
            display.println("--- Waktu Habis! ---");
            display.print("PR ");
            if (subjectSelection >= 0 && subjectSelection < 3) {
                 display.print(subjects[subjectSelection]);
            }
            display.println(" Selesai?");
            display.setTextSize(2);
            display.setCursor(0, 30);
            display.print(menuSelection == 0 ? "> " : "  ");
            display.println("Ya");
            display.setCursor(42, 30);
            display.print(menuSelection == 1 ? "> " : "  ");
            display.println("Tidak");
            break;
            
        case TIME_UP_ADD_TIME: 
            display.println("--- Lanjut Fokus? ---");
            display.println("Ingin menambah timer?");
            display.setTextSize(2);
            display.setCursor(0, 30);
            display.print(menuSelection == 0 ? "> " : "  ");
            display.println("Ya");
            display.setCursor(42, 30);
            display.print(menuSelection == 1 ? "> " : "  ");
            display.println("Tidak");
            break;

        default:
            display.println("Error State");
            break;
    }

    display.display();
}

// --- Main Loop ---
void loop() {
    handleInput();

    // Optimasi refresh display
    if (isTimerActive || currentState == HOME_SCREEN) {
        if (millis() % 500 < 50 || currentState == HOME_SCREEN) { 
             displayMenu();
        }
    } else {
        displayMenu();
    }
}