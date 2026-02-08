# wEui Changelog

## [1.1.0] - 2026-02-08

### Added
- **Configurable button pull mode support with independent configuration per button**
  - Added `wEui_ButtonPullMode_t` enum with `WEUI_BUTTON_PULL_DOWN` and `WEUI_BUTTON_PULL_UP` options
  - Added individual `pullMode` fields to `wEui_ButtonConfig_t` structure for each button:
    - `upPullMode` - Pull mode configuration for UP button
    - `downPullMode` - Pull mode configuration for DOWN button
    - `okPullMode` - Pull mode configuration for OK button
    - `backPullMode` - Pull mode configuration for BACK button
  - Support for both pull-up (active-low) and pull-down (active-high) button configurations
  - Each button can now have its own independent pull mode setting
  - Default mode is `WEUI_BUTTON_PULL_DOWN` for better compatibility with modern hardware

## [1.0.0] - Initial Release

### Features
- Basic UI framework with list management
- Button handling with pull-up configuration only
- Display rendering with U8g2 support
- Page management system
- Status bar and toast notifications
