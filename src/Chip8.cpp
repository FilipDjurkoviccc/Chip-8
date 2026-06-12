#include "Chip8.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::array<uint8_t, Chip8::FontSize> FontSet = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
}

Chip8::Chip8()
    : randomGenerator_(std::random_device{}()) {
    reset();
}

void Chip8::reset() {
    memory_.fill(0);
    registers_.fill(0);
    stack_.fill(0);
    keys_.fill(0);
    display_.fill(0);

    std::copy(FontSet.begin(), FontSet.end(), memory_.begin() + FontStartAddress);

    opcode_ = 0;
    index_ = 0;
    pc_ = ProgramStartAddress;
    sp_ = 0;
    delayTimer_ = 0;
    soundTimer_ = 0;
    drawFlag_ = false;
}

void Chip8::loadRom(const std::string& pathToFile) {
    std::ifstream file(pathToFile, std::ios::binary);
    if (!file) {
        throw std::invalid_argument("Chip8::loadRom: file not found");
    }

    const std::vector<uint8_t> rom{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};

    if (ProgramStartAddress + rom.size() > memory_.size()) {
        throw std::invalid_argument("Chip8::loadRom: file too big");
    }

    std::copy(rom.begin(), rom.end(), memory_.begin() + ProgramStartAddress);
}

void Chip8::cycle() {
    opcode_ = (memory_[pc_ & 0x0FFF] << 8) | memory_[(pc_ + 1) & 0x0FFF];
    pc_ += 2;

    executeInstruction();

    if (delayTimer_ > 0) {
        --delayTimer_;
    }
    if (soundTimer_ > 0) {
        --soundTimer_;
    }
}

void Chip8::setKey(std::size_t key) {
    if (key < keys_.size()) {
        keys_[key] = 1;
    }
}

void Chip8::unsetKey(std::size_t key) {
    if (key < keys_.size()) {
        keys_[key] = 0;
    }
}

const Chip8::DisplayBuffer& Chip8::display() const {
    return display_;
}

bool Chip8::shouldDraw() const {
    return drawFlag_;
}

void Chip8::clearDrawFlag() {
    drawFlag_ = false;
}

void Chip8::LoadRom(const std::string& pathToFile) {
    loadRom(pathToFile);
}

void Chip8::Cycle() {
    cycle();
}

void Chip8::clearDisplay() {
    display_.fill(0);
    drawFlag_ = true;
}

void Chip8::drawSprite(uint8_t x, uint8_t y, uint8_t height) {
    registers_[0xF] = 0;

    for (uint8_t row = 0; row < height; ++row) {
        const uint8_t spriteByte = memory_[index_ + row];

        for (uint8_t col = 0; col < 8; ++col) {
            if ((spriteByte & (0x80 >> col)) == 0) {
                continue;
            }

            const std::size_t wrappedX = (x + col) % ScreenWidth;
            const std::size_t wrappedY = (y + row) % ScreenHeight;
            const std::size_t pixelIndex = wrappedY * ScreenWidth + wrappedX;

            if (display_[pixelIndex] == 1) {
                registers_[0xF] = 1;
            }
            display_[pixelIndex] ^= 1;
        }
    }

    drawFlag_ = true;
}

int Chip8::pressedKey() const {
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i]) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

uint8_t Chip8::xRegister() const {
    return static_cast<uint8_t>((opcode_ & 0x0F00) >> 8);
}

uint8_t Chip8::yRegister() const {
    return static_cast<uint8_t>((opcode_ & 0x00F0) >> 4);
}

uint8_t Chip8::byteOperand() const {
    return static_cast<uint8_t>(opcode_ & 0x00FF);
}

uint16_t Chip8::addressOperand() const {
    return opcode_ & 0x0FFF;
}

void Chip8::executeInstruction() {
    const uint8_t x = xRegister();
    const uint8_t y = yRegister();
    const uint8_t kk = byteOperand();
    const uint16_t nnn = addressOperand();

    switch (opcode_ & 0xF000) {
        case 0x0000:
            switch (opcode_) {
                case 0x00E0:
                    clearDisplay();
                    break;
                case 0x00EE:
                    if (sp_ > 0) {
                        --sp_;
                        pc_ = stack_[sp_];
                    }
                    break;
                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode_ << std::dec << '\n';
                    break;
            }
            break;

        case 0x1000:
            pc_ = nnn;
            break;

        case 0x2000:
            stack_[sp_] = pc_;
            ++sp_;
            pc_ = nnn;
            break;

        case 0x3000:
            if (registers_[x] == kk) {
                pc_ += 2;
            }
            break;

        case 0x4000:
            if (registers_[x] != kk) {
                pc_ += 2;
            }
            break;

        case 0x5000:
            if ((opcode_ & 0x000F) == 0 && registers_[x] == registers_[y]) {
                pc_ += 2;
            }
            break;

        case 0x6000:
            registers_[x] = kk;
            break;

        case 0x7000:
            registers_[x] += kk;
            break;

        case 0x8000:
            switch (opcode_ & 0x000F) {
                case 0x0:
                    registers_[x] = registers_[y];
                    break;
                case 0x1:
                    registers_[x] |= registers_[y];
                    registers_[0xF] = 0;
                    break;
                case 0x2:
                    registers_[x] &= registers_[y];
                    registers_[0xF] = 0;
                    break;
                case 0x3:
                    registers_[x] ^= registers_[y];
                    registers_[0xF] = 0;
                    break;
                case 0x4: {
                    const uint16_t sum = registers_[x] + registers_[y];
                    registers_[0xF] = sum > 0xFF;
                    registers_[x] = static_cast<uint8_t>(sum);
                    break;
                }
                case 0x5:
                    registers_[0xF] = registers_[x] >= registers_[y];
                    registers_[x] -= registers_[y];
                    break;
                case 0x6:
                    registers_[0xF] = registers_[x] & 0x1;
                    registers_[x] >>= 1;
                    break;
                case 0x7:
                    registers_[0xF] = registers_[y] >= registers_[x];
                    registers_[x] = registers_[y] - registers_[x];
                    break;
                case 0xE:
                    registers_[0xF] = (registers_[x] & 0x80) >> 7;
                    registers_[x] <<= 1;
                    break;
                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode_ << std::dec << '\n';
                    break;
            }
            break;

        case 0x9000:
            if ((opcode_ & 0x000F) == 0 && registers_[x] != registers_[y]) {
                pc_ += 2;
            }
            break;

        case 0xA000:
            index_ = nnn;
            break;

        case 0xB000:
            pc_ = nnn + registers_[0];
            break;

        case 0xC000: {
            std::uniform_int_distribution<int> distribution(0, 255);
            registers_[x] = static_cast<uint8_t>(distribution(randomGenerator_)) & kk;
            break;
        }

        case 0xD000:
            drawSprite(registers_[x], registers_[y], opcode_ & 0x000F);
            break;

        case 0xE000:
            switch (opcode_ & 0x00FF) {
                case 0x9E:
                    if (registers_[x] < keys_.size() && keys_[registers_[x]]) {
                        pc_ += 2;
                    }
                    break;
                case 0xA1:
                    if (registers_[x] < keys_.size() && !keys_[registers_[x]]) {
                        pc_ += 2;
                    }
                    break;
                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode_ << std::dec << '\n';
                    break;
            }
            break;

        case 0xF000:
            switch (opcode_ & 0x00FF) {
                case 0x07:
                    registers_[x] = delayTimer_;
                    break;
                case 0x0A: {
                    const int key = pressedKey();
                    if (key == -1) {
                        pc_ -= 2;
                    } else {
                        registers_[x] = static_cast<uint8_t>(key);
                    }
                    break;
                }
                case 0x15:
                    delayTimer_ = registers_[x];
                    break;
                case 0x18:
                    soundTimer_ = registers_[x];
                    break;
                case 0x1E:
                    registers_[0xF] = index_ + registers_[x] > 0x0FFF;
                    index_ += registers_[x];
                    break;
                case 0x29:
                    index_ = FontStartAddress + (5 * registers_[x]);
                    break;
                case 0x33:
                    memory_[index_] = registers_[x] / 100;
                    memory_[index_ + 1] = (registers_[x] / 10) % 10;
                    memory_[index_ + 2] = registers_[x] % 10;
                    break;
                case 0x55:
                    for (uint8_t i = 0; i <= x; ++i) {
                        memory_[index_ + i] = registers_[i];
                    }
                    index_ += x + 1;
                    break;
                case 0x65:
                    for (uint8_t i = 0; i <= x; ++i) {
                        registers_[i] = memory_[index_ + i];
                    }
                    index_ += x + 1;
                    break;
                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode_ << std::dec << '\n';
                    break;
            }
            break;

        default:
            std::cout << "Unknown opcode: " << std::hex << opcode_ << std::dec << '\n';
            break;
    }
}
