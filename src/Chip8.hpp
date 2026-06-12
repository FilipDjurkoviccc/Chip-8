#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

class Chip8 {
  public:
    static constexpr std::size_t MemorySize = 4096;
    static constexpr std::size_t RegisterCount = 16;
    static constexpr std::size_t StackSize = 16;
    static constexpr std::size_t KeyCount = 16;
    static constexpr std::size_t ScreenWidth = 64;
    static constexpr std::size_t ScreenHeight = 32;
    static constexpr std::size_t ScreenSize = ScreenWidth * ScreenHeight;
    static constexpr std::size_t FontSize = 80;

    using DisplayBuffer = std::array<uint8_t, ScreenSize>;

    Chip8();

    void loadRom(const std::string& pathToFile);
    void cycle();
    void setKey(std::size_t key);
    void unsetKey(std::size_t key);

    const DisplayBuffer& display() const;
    bool shouldDraw() const;
    void clearDrawFlag();

    // Compatibility with the original API names.
    void LoadRom(const std::string& pathToFile);
    void Cycle();

  private:
    static constexpr uint16_t ProgramStartAddress = 0x200;
    static constexpr uint16_t FontStartAddress = 0x000;

    void reset();
    void executeInstruction();
    void clearDisplay();
    void drawSprite(uint8_t x, uint8_t y, uint8_t height);
    int pressedKey() const;

    uint8_t xRegister() const;
    uint8_t yRegister() const;
    uint8_t byteOperand() const;
    uint16_t addressOperand() const;

    std::array<uint8_t, MemorySize> memory_{};
    std::array<uint8_t, RegisterCount> registers_{};
    std::array<uint16_t, StackSize> stack_{};
    std::array<uint8_t, KeyCount> keys_{};
    DisplayBuffer display_{};

    uint16_t opcode_{0};
    uint16_t index_{0};
    uint16_t pc_{ProgramStartAddress};
    uint8_t sp_{0};
    uint8_t delayTimer_{0};
    uint8_t soundTimer_{0};
    bool drawFlag_{false};

    std::mt19937 randomGenerator_;
};
