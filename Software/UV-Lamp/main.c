#include "LCD_Test.h"
#include "LCD_1in69.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/pwm.h" 

// ========================================
// KONFIGURACJA
// ========================================
#define TARGET_FPS 30
#define FRAME_TIME_MS (1000 / TARGET_FPS)

// GPIO PINY
#define ENC_A_PIN 10
#define ENC_B_PIN 11
#define BUTT1_PIN 9
#define BUTT2_PIN 8
#define PWM_CH1_PIN 13
#define PWM_CH2_PIN 14

// KONFIGURACJA PWM
#define PWM_FREQUENCY 1000  // 1 kHz
#define PWM_MAX_DUTY 65535  // 16-bit PWM (0-65535)

// ========================================
// KONFIGURACJA FLASH MEMORY
// ========================================
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)  // Ostatni sektor flash
#define FLASH_MAGIC_NUMBER 0xABCD1234  // Numer magiczny do weryfikacji danych

typedef struct {
    uint32_t magic;              // Numer magiczny do weryfikacji
    uint8_t timer_value;         // Wartość timera (0-180)
    uint8_t power_value;         // Wartość mocy (0-100)
    uint16_t reserved;           // Zarezerwowane dla przyszłych użyć
} flash_config_t;

// Konfiguracja przycisków
#define DEBOUNCE_TIME_MS 10
#define LONG_PRESS_TIME_MS 1000

// Konfiguracja timera
#define ARC_UPDATE_INTERVAL_MS 100  // Aktualizacja arcu co 100ms dla płynności

// ========================================
// BUFORY DLA LVGL
// ========================================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LCD_1IN69_WIDTH * 70];

// ========================================
// ZMIENNE GLOBALNE
// ========================================
static uint8_t current_screen = 0;
static uint8_t configured_timer_value = 0;
static uint8_t configured_power_value = 0;

// Zmienne dla enkodera
static volatile int8_t encoder_delta = 0;
static volatile uint8_t encoder_last_state = 0;

// Zmienne dla przycisków
static volatile bool butt1_pressed = false;
static volatile bool butt2_pressed = false;
static volatile uint64_t butt1_press_time = 0;
static volatile uint64_t butt2_press_time = 0;
static volatile bool butt1_handled = false;
static volatile bool butt2_handled = false;

// Tryb edycji
static bool edit_mode = false;

// ========================================
// ZMIENNE DLA TIMERA
// ========================================
static bool timer_running = false;
static uint32_t timer_remaining_ms = 0;  // Pozostały czas w milisekundach
static uint64_t last_timer_update = 0;   // Ostatnia aktualizacja timera (ms)
static uint64_t last_arc_update = 0;     // Ostatnia aktualizacja arcu (ms)

// ZMIENNE DLA PWM
static uint pwm_slice_ch1;
static uint pwm_slice_ch2;
static uint pwm_chan_ch1;
static uint pwm_chan_ch2;

// ========================================
// INICJALIZACJA PWM
// ========================================
void init_pwm(void)
{
    // Konfiguracja PWM dla CH1 (pin 13)
    gpio_set_function(PWM_CH1_PIN, GPIO_FUNC_PWM);
    pwm_slice_ch1 = pwm_gpio_to_slice_num(PWM_CH1_PIN);
    pwm_chan_ch1 = pwm_gpio_to_channel(PWM_CH1_PIN);
    
    // Konfiguracja PWM dla CH2 (pin 14)
    gpio_set_function(PWM_CH2_PIN, GPIO_FUNC_PWM);
    pwm_slice_ch2 = pwm_gpio_to_slice_num(PWM_CH2_PIN);
    pwm_chan_ch2 = pwm_gpio_to_channel(PWM_CH2_PIN);
    
    // Ustawienie częstotliwości PWM
    // Clock system: 125 MHz
    // Dla 1 kHz: wrap = 125000 / 1000 = 125
    uint32_t clock_freq = 125000000;  // 125 MHz
    uint32_t divider = clock_freq / (PWM_FREQUENCY * PWM_MAX_DUTY);
    if (divider < 1) divider = 1;
    
    // Konfiguracja slice dla CH1
    pwm_config config_ch1 = pwm_get_default_config();
    pwm_config_set_clkdiv(&config_ch1, (float)divider);
    pwm_config_set_wrap(&config_ch1, PWM_MAX_DUTY - 1);
    pwm_init(pwm_slice_ch1, &config_ch1, false); 
    
    // Konfiguracja slice dla CH2
    pwm_config config_ch2 = pwm_get_default_config();
    pwm_config_set_clkdiv(&config_ch2, (float)divider);
    pwm_config_set_wrap(&config_ch2, PWM_MAX_DUTY - 1);
    pwm_init(pwm_slice_ch2, &config_ch2, false); 
    
    // Ustaw początkowe duty cycle na 0
    pwm_set_chan_level(pwm_slice_ch1, pwm_chan_ch1, 0);
    pwm_set_chan_level(pwm_slice_ch2, pwm_chan_ch2, 0);
}

// ========================================
// AKTUALIZACJA DUTY CYCLE PWM
// ========================================
void update_pwm_duty(uint8_t power_percent)
{
    // Przelicz procent mocy (0-100) na duty cycle (0-65535)
    uint16_t duty = (power_percent * PWM_MAX_DUTY) / 100;
    
    // Ustaw duty cycle dla obu kanałów
    pwm_set_chan_level(pwm_slice_ch1, pwm_chan_ch1, duty);
    pwm_set_chan_level(pwm_slice_ch2, pwm_chan_ch2, duty);
}

// ========================================
// START PWM
// ========================================
void start_pwm(void)
{
    update_pwm_duty(configured_power_value);
    pwm_set_enabled(pwm_slice_ch1, true);
    pwm_set_enabled(pwm_slice_ch2, true);
}

// ========================================
// STOP PWM
// ========================================
void stop_pwm(void)
{
    pwm_set_enabled(pwm_slice_ch1, false);
    pwm_set_enabled(pwm_slice_ch2, false);
    
    // Ustaw duty cycle na 0 dla bezpieczeństwa
    pwm_set_chan_level(pwm_slice_ch1, pwm_chan_ch1, 0);
    pwm_set_chan_level(pwm_slice_ch2, pwm_chan_ch2, 0);
}

// ========================================
// ODCZYT KONFIGURACJI Z FLASH
// ========================================
bool load_config_from_flash(void)
{
    const flash_config_t *flash_data = (const flash_config_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Sprawdź numer magiczny
    if (flash_data->magic == FLASH_MAGIC_NUMBER) {
        // Dane są prawidłowe
        configured_timer_value = flash_data->timer_value;
        configured_power_value = flash_data->power_value;
        
        // Walidacja zakresu (na wypadek uszkodzonych danych)
        if (configured_timer_value > 180) configured_timer_value = 0;
        if (configured_power_value > 100) configured_power_value = 0;
        
        return true;
    }
    
    // Brak prawidłowych danych - użyj wartości domyślnych
    configured_timer_value = 0;
    configured_power_value = 0;
    return false;
}

// ========================================
// ZAPIS KONFIGURACJI DO FLASH
// ========================================
void save_config_to_flash(void)
{
    flash_config_t config;
    config.magic = FLASH_MAGIC_NUMBER;
    config.timer_value = configured_timer_value;
    config.power_value = configured_power_value;
    config.reserved = 0;
    
    // Bufor musi być wyrównany do rozmiaru strony flash (256 bajtów)
    uint8_t buffer[FLASH_PAGE_SIZE];
    memset(buffer, 0xFF, FLASH_PAGE_SIZE);  // Wypełnij 0xFF (stan skasowanej pamięci)
    memcpy(buffer, &config, sizeof(flash_config_t));
    
    // Wyłącz przerwania podczas operacji flash
    uint32_t ints = save_and_disable_interrupts();
    
    // Skasuj sektor (4096 bajtów)
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    
    // Zapisz dane (256 bajtów)
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_PAGE_SIZE);
    
    // Przywróć przerwania
    restore_interrupts(ints);
}

// ========================================
// FUNKCJA WYŚWIETLANIA OBSZARU LCD
// ========================================
void LCD_1IN69_DisplayArea(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD *Image)
{
    UWORD width = Xend - Xstart + 1;
    UWORD height = Yend - Ystart + 1;
    UWORD total_pixels = width * height;
    
    LCD_1IN69_SetWindows(Xstart, Ystart, Xend, Yend);
    
    DEV_Digital_Write(LCD_DC_PIN, 1);
    DEV_Digital_Write(LCD_CS_PIN, 0);
    
    DEV_SPI_Write_nByte((uint8_t *)Image, total_pixels * 2);
    
    DEV_Digital_Write(LCD_CS_PIN, 1);
}

// ========================================
// CALLBACK FLUSH DLA LVGL
// ========================================
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    LCD_1IN69_DisplayArea(area->x1, area->y1, area->x2, area->y2, (UWORD *)color_p);
    lv_disp_flush_ready(disp);
}

// ========================================
// TIMER CALLBACK DLA LVGL TICK
// ========================================
bool lvgl_timer_callback(struct repeating_timer *t)
{
    lv_tick_inc(1);
    return true;
}

// ========================================
// ZARZĄDZANIE WIDOCZNOŚCIĄ ARKÓW
// ========================================
void update_arc_visibility(void)
{
    if (edit_mode) {
        // Pokaż arki w trybie edycji
        if (current_screen == 1) {
            lv_obj_clear_flag(ui_ArcPowerValue, LV_OBJ_FLAG_HIDDEN);
        } else if (current_screen == 2) {
            lv_obj_clear_flag(ui_ArcTimeValue, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // Ukryj arki poza trybem edycji
        lv_obj_add_flag(ui_ArcPowerValue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_ArcTimeValue, LV_OBJ_FLAG_HIDDEN);
        
        // Zapisz konfigurację do flash przy wyjściu z trybu edycji
        save_config_to_flash(); 
    }
}

// ========================================
// AKTUALIZACJA WYŚWIETLACZA TIMERA
// ========================================
void update_timer_display(void)
{
    char timerText[16];
    int minutes = configured_timer_value / 60;
    int seconds = configured_timer_value % 60;
    snprintf(timerText, sizeof(timerText), "%02d:%02d", minutes, seconds);
    lv_label_set_text(ui_LabelConfigTimerValue, timerText);
    int16_t arc_val = configured_timer_value * 10 / 18;
    lv_arc_set_value(ui_ArcTimeValue, arc_val);
}

// ========================================
// AKTUALIZACJA WYŚWIETLACZA MOCY
// ========================================
void update_power_display(void)
{
    char powerText[4];
    snprintf(powerText, sizeof(powerText), "%d", configured_power_value);
    lv_arc_set_value(ui_ArcPowerValue, configured_power_value);
    lv_label_set_text(ui_LabelPowerValue, powerText);
}

// ========================================
// AKTUALIZACJA ARCU NA EKRANIE GŁÓWNYM
// ========================================
void update_main_arc(void)
{
    if (configured_timer_value > 0) {
        // Oblicz procent wypełnienia arcu na podstawie milisekund
        uint32_t total_ms = configured_timer_value * 1000;
        int16_t arc_percent = (timer_remaining_ms * 100) / total_ms;
        
        // Ogranicz do zakresu 0-100
        if (arc_percent > 100) arc_percent = 100;
        if (arc_percent < 0) arc_percent = 0;
        
        lv_arc_set_value(ui_ArcMainTimerValue, arc_percent);
    } else {
        lv_arc_set_value(ui_ArcMainTimerValue, 0);
    }
}

// ========================================
// AKTUALIZACJA ETYKIETY CZASU NA EKRANIE GŁÓWNYM
// ========================================
void update_main_label(void)
{
    char timerText[16];
    uint32_t seconds = timer_remaining_ms / 1000;
    int minutes = seconds / 60;
    int secs = seconds % 60;
    snprintf(timerText, sizeof(timerText), "%02d:%02d", minutes, secs);
    lv_label_set_text(ui_LabelMainTimerValue, timerText);
}

// ========================================
// AKTUALIZACJA EKRANU GŁÓWNEGO (PEŁNA)
// ========================================
void update_main_display(void)
{
    // Aktualizacja wartości mocy
    char powerText[8];
    snprintf(powerText, sizeof(powerText), "%d%%", configured_power_value);
    lv_label_set_text(ui_LabelPowerValueSet, powerText);
    
    // Aktualizacja wartości timera i arcu
    update_main_label();
    update_main_arc();
}

// ========================================
// RESET TIMERA
// ========================================
void reset_timer(void)
{
    timer_running = false;
    timer_remaining_ms = configured_timer_value * 1000;
    last_timer_update = 0;
    last_arc_update = 0;
    update_main_display();
}

// ========================================
// START/PAUSE TIMERA
// ========================================
void toggle_timer(void)
{
    if (timer_running) {
        // Zatrzymaj timer
        timer_running = false;
        stop_pwm();
    } else {
        // Uruchom timer
        if (timer_remaining_ms > 0) {
            timer_running = true;
            last_timer_update = time_us_64() / 1000;
            last_arc_update = last_timer_update;
            
            start_pwm();
        }
    }
}

// ========================================
// OBSŁUGA TIMERA W GŁÓWNEJ PĘTLI
// ========================================
void process_timer(void)
{
    if (!timer_running || current_screen != 0) {
        return;
    }
    
    uint64_t current_time = time_us_64() / 1000; // czas w ms
    uint64_t elapsed = current_time - last_timer_update;
    
    if (elapsed > 0) {
        last_timer_update = current_time;
        
        // Odejmij upłynięty czas
        if (timer_remaining_ms > elapsed) {
            timer_remaining_ms -= elapsed;
        } else {
            // Timer zakończony
            timer_remaining_ms = 0;
            timer_running = false;
            
            // Zatrzymaj PWM po zakończeniu timera
            stop_pwm();  
            
            // Automatyczny reset do wartości początkowej
            timer_remaining_ms = configured_timer_value * 1000;
            update_main_display();
            
            return; // Zakończ funkcję, nie aktualizuj już wyświetlacza
        }
        
        // Aktualizuj etykietę co sekundę (gdy zmienia się wartość w sekundach)
        static uint32_t last_displayed_seconds = 0xFFFFFFFF;
        uint32_t current_seconds = timer_remaining_ms / 1000;
        
        if (current_seconds != last_displayed_seconds) {
            update_main_label();
            last_displayed_seconds = current_seconds;
        }
        
        // Aktualizuj arc częściej (co ARC_UPDATE_INTERVAL_MS)
        if (current_time - last_arc_update >= ARC_UPDATE_INTERVAL_MS) {
            update_main_arc();
            last_arc_update = current_time;
        }
    }
}

// ========================================
// WSPÓLNY CALLBACK GPIO (ENKODER I PRZYCISKI)
// ========================================
void gpio_callback(uint gpio, uint32_t events)
{
    // Obsługa enkodera
    if (gpio == ENC_A_PIN || gpio == ENC_B_PIN) {
        uint8_t enc_a = gpio_get(ENC_A_PIN);
        uint8_t enc_b = gpio_get(ENC_B_PIN);
        uint8_t current_state = (enc_a << 1) | enc_b;
        
        // Dekodowanie kierunku obrotu (Gray code)
        uint8_t combined = (encoder_last_state << 2) | current_state;
        
        switch(combined) {
            case 0b0001: case 0b0111: case 0b1110: case 0b1000:
                encoder_delta++;
                break;
            case 0b0010: case 0b0100: case 0b1101: case 0b1011:
                encoder_delta--;
                break;
        }
        
        encoder_last_state = current_state;
    }
    // Obsługa przycisków
    else if (gpio == BUTT1_PIN) {
        uint64_t current_time = time_us_64() / 1000; // czas w ms
        
        if (events & GPIO_IRQ_EDGE_FALL) {
            // Przycisk wciśnięty
            butt1_pressed = true;
            butt1_press_time = current_time;
            butt1_handled = false;
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            // Przycisk zwolniony
            butt1_pressed = false;
        }
    } else if (gpio == BUTT2_PIN) {
        uint64_t current_time = time_us_64() / 1000; // czas w ms
        
        if (events & GPIO_IRQ_EDGE_FALL) {
            butt2_pressed = true;
            butt2_press_time = current_time;
            butt2_handled = false;
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            butt2_pressed = false;
        }
    }
}

// ========================================
// INICJALIZACJA GPIO (ENKODER I PRZYCISKI)
// ========================================
void init_gpio(void)
{
    // Inicjalizacja enkodera
    gpio_init(ENC_A_PIN);
    gpio_init(ENC_B_PIN);
    gpio_set_dir(ENC_A_PIN, GPIO_IN);
    gpio_set_dir(ENC_B_PIN, GPIO_IN);
    gpio_pull_up(ENC_A_PIN);
    gpio_pull_up(ENC_B_PIN);
    
    // Inicjalizacja przycisków
    gpio_init(BUTT1_PIN);
    gpio_init(BUTT2_PIN);
    gpio_set_dir(BUTT1_PIN, GPIO_IN);
    gpio_set_dir(BUTT2_PIN, GPIO_IN);
    gpio_pull_up(BUTT1_PIN);
    gpio_pull_up(BUTT2_PIN);
    
    // Odczytaj początkowy stan enkodera
    encoder_last_state = (gpio_get(ENC_A_PIN) << 1) | gpio_get(ENC_B_PIN);
    
    // Ustawienie przerwań z jednym wspólnym callbackiem
    gpio_set_irq_enabled_with_callback(ENC_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(ENC_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTT1_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTT2_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

// ========================================
// OBSŁUGA ENKODERA W GŁÓWNEJ PĘTLI
// ========================================
void process_encoder(void)
{
    if (encoder_delta == 0) {
        return;
    }
    
    int8_t delta = encoder_delta;
    encoder_delta = 0;
    
    // Obsługa enkodera tylko w trybie edycji
    if (!edit_mode) {
        return;
    }
    
    if (current_screen == 1) {
        // Ekran konfiguracji mocy
        int16_t new_value = configured_power_value + delta;
        if (new_value < 0) new_value = 0;
        if (new_value > 100) new_value = 100;
        configured_power_value = new_value;
        update_power_display();
        
    } else if (current_screen == 2) {
        // Ekran konfiguracji timera
        int16_t new_value = configured_timer_value + delta;
        if (new_value < 0) new_value = 0;
        if (new_value > 180) new_value = 180;
        configured_timer_value = new_value;
        update_timer_display();
    }
}

// ========================================
// OBSŁUGA PRZYCISKÓW W GŁÓWNEJ PĘTLI
// ========================================
void process_buttons(void)
{
    uint64_t current_time = time_us_64() / 1000; // czas w ms
    
    // ========================================
    // Obsługa BUTT1 (zmiana ekranów / tryb edycji)
    // ========================================
    if (butt1_pressed && !butt1_handled) {
        uint64_t press_duration = current_time - butt1_press_time;
        
        // Long press - wejście/wyjście z trybu edycji
        if (press_duration >= LONG_PRESS_TIME_MS) {
            butt1_handled = true;
            
            // Long press działa tylko na ekranach konfiguracji
            if (current_screen == 1 || current_screen == 2) {
                edit_mode = !edit_mode;
                update_arc_visibility(); // Aktualizuj widoczność arków
            }
        }
    } else if (!butt1_pressed && butt1_press_time > 0) {
        // Przycisk został zwolniony
        uint64_t press_duration = current_time - butt1_press_time;
        
        // Short press - zmiana ekranu
        // BLOKADA: nie można zmieniać ekranu podczas pracy timera, w trybie edycji lub po long press
        if (press_duration < LONG_PRESS_TIME_MS && 
            press_duration > DEBOUNCE_TIME_MS && 
            !edit_mode && 
            !timer_running) {  // <-- DODANA BLOKADA
            
            current_screen++;
            if (current_screen > 2) {
                current_screen = 0;
            }
            
            switch(current_screen) {
                case 0:
                    lv_scr_load_anim(ui_ScreenMain, 
                                    LV_SCR_LOAD_ANIM_MOVE_LEFT, 
                                    500, 0, false);
                    // Zresetuj timer przy powrocie do ekranu głównego
                    reset_timer();
                    break;
                case 1:
                    lv_scr_load_anim(ui_ScreenPowerSetting, 
                                    LV_SCR_LOAD_ANIM_MOVE_LEFT, 
                                    500, 0, false);
                    update_arc_visibility(); // Aktualizuj po zmianie ekranu
                    break;
                case 2:
                    lv_scr_load_anim(ui_ScreenTimerSetting, 
                                    LV_SCR_LOAD_ANIM_MOVE_LEFT, 
                                    500, 0, false);
                    update_arc_visibility(); // Aktualizuj po zmianie ekranu
                    break;
            }
        }
        
        butt1_press_time = 0;
        butt1_handled = false;
    }
    
    // ========================================
    // Obsługa BUTT2 (timer play/pause/reset)
    // ========================================
    if (butt2_pressed && !butt2_handled) {
        uint64_t press_duration = current_time - butt2_press_time;
        
        // Long press - reset timera (tylko na ekranie głównym)
        if (press_duration >= LONG_PRESS_TIME_MS) {
            butt2_handled = true;
            
            if (current_screen == 0) {
                reset_timer();
            }
        }
    } else if (!butt2_pressed && butt2_press_time > 0) {
        // Przycisk został zwolniony
        uint64_t press_duration = current_time - butt2_press_time;
        
        // Short press - play/pause (tylko jeśli nie było long press i jesteśmy na ekranie głównym)
        if (press_duration < LONG_PRESS_TIME_MS && press_duration > DEBOUNCE_TIME_MS) {
            if (current_screen == 0 && !butt2_handled) {
                toggle_timer();
            }
        }
        
        butt2_press_time = 0;
        butt2_handled = false;
    }
}

// ========================================
// INICJALIZACJA HARDWARE
// ========================================
bool init_hardware(void)
{
    if (DEV_Module_Init() != 0) {
        return false;
    }
    
    DEV_SET_PWM(100);
    LCD_1IN69_Init(VERTICAL);
    LCD_1IN69_Clear(WHITE);
    
    init_gpio();
    init_pwm();
    
    return true;
}

// ========================================
// INICJALIZACJA LVGL
// ========================================
bool init_lvgl(void)
{
    lv_init();
    
    // Inicjalizacja bufora rysowania
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_1IN69_WIDTH * 70);
    
    // Inicjalizacja sterownika wyświetlacza
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    disp_drv.hor_res = LCD_1IN69_WIDTH;
    disp_drv.ver_res = LCD_1IN69_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    if (disp == NULL) {
        return false;
    }
    
    // Timer dla LVGL tick (1ms)
    static struct repeating_timer timer;
    add_repeating_timer_ms(1, lvgl_timer_callback, NULL, &timer);
    
    return true;
}

// ========================================
// INICJALIZACJA UI
// ========================================
void init_ui_timers(void)
{
    // Załaduj konfigurację z flash
    load_config_from_flash();
    
    ui_init();
    
    // Inicjalizacja początkowych wartości
    update_timer_display();
    update_power_display();
    
    // Inicjalizacja timera głównego
    timer_remaining_ms = configured_timer_value * 1000;
    
    // Ukryj arki na starcie (nie jesteśmy w trybie edycji)
    lv_obj_add_flag(ui_ArcPowerValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ArcTimeValue, LV_OBJ_FLAG_HIDDEN);
    
    // Załaduj ekran główny i zaktualizuj jego wartości
    lv_scr_load(ui_ScreenMain);
    update_main_display();
}

// ========================================
// GŁÓWNA PĘTLA Z KONTROLĄ FPS
// ========================================
void main_loop(void)
{
    while (1) {
        uint64_t frame_start = time_us_64();
        
        // Obsługa timera
        process_timer();
        
        // Obsługa enkodera
        process_encoder();
        
        // Obsługa przycisków
        process_buttons();
        
        // Obsługa LVGL (renderowanie, timery, eventy)
        lv_timer_handler();
        
        // Oblicz czas renderowania ramki
        uint64_t frame_time = (time_us_64() - frame_start) / 1000;
        
        // Poczekaj do następnej ramki (kontrola FPS)
        if (frame_time < FRAME_TIME_MS) {
            sleep_ms(FRAME_TIME_MS - frame_time);
        }
    }
}

// ========================================
// MAIN
// ========================================
int main(void) 
{
    sleep_ms(100);  // Stabilizacja po starcie
    
    // Inicjalizacja hardware
    if (!init_hardware()) {
        while(1) { tight_loop_contents(); }  // Zatrzymaj się w przypadku błędu
    }
    
    // Inicjalizacja LVGL
    if (!init_lvgl()) {
        while(1) { tight_loop_contents(); }  // Zatrzymaj się w przypadku błędu
    }
    
    // Inicjalizacja UI
    init_ui_timers();
    
    // Główna pętla
    main_loop();
    
    // Cleanup (nigdy nie powinno się tu dojść)
    DEV_Module_Exit();
    return 0;
}
