#ifndef SCREENS_H
#define SCREENS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ───────────────────────────────────────────────────────────────────────────
 * Screen Rendering Functions
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * Render the splash screen
 * Shows device name, "Initializing..." subtitle, and animated loading bar
 */
void screen_splash(void);

/**
 * Render the dashboard screen (main screen)
 * Shows: stage indicator, device count, RSSI bar, GPS placeholder, heading placeholder
 */
void screen_dashboard(void);

/**
 * Render the radar/direction screen
 * Shows: compass rose, directional arrow, signal rings, heading degrees
 */
void screen_radar(void);

/**
 * Render the settings screen
 * Shows: list of settings with cursor arrow and highlighted selection
 */
void screen_settings(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREENS_H */
