#pragma once
#include <cstdint>
#include <array>

class Ppu {
public:
    enum class Mode {
        HBlank = 0,
        VBlank = 1,
        OamSearch = 2,
        PixelTransfer = 3
    };

    Ppu();
    ~Ppu() = default;

    /// Reads a byte from VRAM, OAM or PPU registers.
    uint8_t read(uint16_t address) const;
    /// Reads a byte to VRAM, OAM or PPU registers.
    void write(uint16_t address, uint8_t value);

    /// Advances PPU's state machine using CPU's M-cycles.
    void tick(uint8_t cycles);

    /// Returns the frame buffer for the frontend.
    const std::array<uint32_t, 160 * 144>& get_frame_buffer() const { return frame_buffer; }

    bool is_vblank_interrupt_requested() const { return vblank_interrupt_requested; }
    void clear_vblank_interrupt() { vblank_interrupt_requested = false; }

    bool is_stat_interrupt_requested() const { return stat_interrupt_requested; }
    void clear_stat_interrupt() { stat_interrupt_requested = false; }

    /// Sets the palette for non-color games
    void set_palette(const std::array<uint32_t, 4>& new_colors) { colors = new_colors; }

private:
    std::array<uint8_t, 8192> vram{}; // RAM, 8 KB  (0x8000 - 0x9FFF)
    std::array<uint8_t, 160> oam{};   // OAM, 160 B (0xFE00 - 0xFE9F)

    uint8_t lcdc{0x91}; // LCD Control
    uint8_t stat{0x85}; // LCD Status
    uint8_t scy{0x00};  // Scroll Y
    uint8_t scx{0x00};  // Scroll X
    uint8_t ly{0x00};   // LCD Y Coordinate
    uint8_t lyc{0x00};  // LY Compare
    uint8_t bgp{0xFC};  // Background Palette
    uint8_t obp0{0xFF}; // Object Palette 0
    uint8_t obp1{0xFF}; // Object Palette 1
    uint8_t wy{0x00};   // Window Y
    uint8_t wx{0x00};   // Window X

    uint16_t cycles_accumulator{0};

    bool vblank_interrupt_requested{false};
    bool stat_interrupt_requested{false};

    std::array<uint32_t, 160 * 144> frame_buffer{};
    std::array<uint32_t, 4> colors = { 0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820 };

    uint8_t window_line{0};

    void change_mode(Mode new_mode);
    void check_lyc();

    void render_scanline();
    void render_background();
    void render_window();
    void render_sprites();
};