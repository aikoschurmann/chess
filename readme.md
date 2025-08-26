# ♟️ High-Performance Chess Engine

A professional-grade chess engine written in C, featuring optimized bitboard representation, magic bitboard attack generation, and comprehensive move validation. Built for both performance and correctness with extensive testing infrastructure.

## 🎯 Key Features

### Core Engine
- **Magic Bitboard System**: Ultra-fast O(1) sliding piece attack generation using magic numbers
- **Optimized Move Generation**: Staged move generation (captures, quiet moves, special moves) with ~60M nodes/sec performance
- **Complete Rule Implementation**: Full chess rules including castling, en passant, promotions, and king safety
- **Advanced Move Application**: Fast make/unmake system with proper state restoration for search algorithms

### Development & Testing
- **Perft Testing**: Comprehensive performance testing with divide functionality for debugging
- **Stockfish Validation**: Reference engine comparison for move generation correctness
- **Professional Build System**: Optimized compilation with LTO and native architecture targeting
- **Modular Architecture**: Clean separation between core engine, GUI, and utilities

### User Interface
- **SDL2-Based GUI**: Interactive chess board with piece movement and move highlighting
- **Command Line Interface**: Powerful CLI for testing, benchmarking, and debugging
- **Debug Visualization**: Bitboard visualization for development and analysis

## 📊 Performance Metrics

- **Move Generation**: ~60 million nodes/second (perft depth 5)
- **Code Base**: 1,858 lines of C code + 108K lines of generated magic tables
- **Architecture**: Optimized for modern 64-bit processors with native instruction usage

## 🛠️ Build & Installation

### Prerequisites
- GCC or Clang compiler with C99 support
- SDL2 and SDL2_image libraries (for GUI)
- Make build system

### macOS Installation
```bash
# Install SDL2 via Homebrew
brew install sdl2 sdl2_image

# Clone and build
git clone <repository-url>
cd chess
make
```

### Linux Installation
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-image-dev

# Fedora/RHEL
sudo dnf install SDL2-devel SDL2_image-devel

# Build
make
```

## 🚀 Usage

### GUI Mode
Launch the interactive chess interface:
```bash
./out/chess
```

**Controls:**
- Click to select pieces and make moves
- `R` key to reset the board
- Debug keys for bitboard visualization

### Command Line Interface
```bash
# Run performance tests
./out/chess --perft <depth>

# Analyze move generation (with move breakdown)
./out/chess --divide <depth>

# Show help
./out/chess --help
```

### Performance Testing Examples
```bash
# Quick validation (depth 4)
./out/chess --perft 4

# Full benchmark (depth 6)
./out/chess --perft 6

# Debug specific positions
./out/chess --divide 3
```

## 🏗️ Architecture

### Core Components

```
📁 Core Engine
├── Magic Bitboards     → O(1) attack generation
├── Move Generator      → Staged move generation  
├── Move Application    → Fast make/unmake system
├── Board Representation → 64-bit bitboard arrays
└── Rule Validation     → King safety & legality

📁 Performance Layer
├── Magic Number Generator → Generates optimal magic constants
├── Perft Testing          → Validates correctness & benchmarks
├── Divide Analysis        → Move-by-move debugging
└── Stockfish Integration  → Reference validation

📁 User Interface
├── SDL2 GUI              → Interactive board interface
├── Command Line Tools    → Testing & analysis utilities
├── Debug Visualization   → Bitboard display system
└── Input Handling        → Mouse/keyboard events
```

### File Structure
```
include/          → Header files and interfaces
├── bitboard.h           → Bitboard utilities
├── magic_*.h            → Magic bitboard system
├── move_gen_optimized.h → Optimized move generation
├── chess_board.h        → Board representation
└── gui.h               → User interface

src/              → Implementation files  
├── magic_bitboards.c    → Magic number generation
├── move_gen_optimized.c → High-performance move gen
├── move_apply_optimized.c → Fast move application
├── perft.c             → Performance testing
└── main.c              → Application entry point

Generated Files   → Auto-generated magic tables
├── magic_data_generated.h     → Magic constants
├── magic_attacks_*.h          → Precomputed attacks
└── Magic number databases
```


### Reference Validation
Cross-validated against Stockfish for move generation correctness:
- ✅ All standard positions pass
- ✅ Complex tactical positions verified  
- ✅ Edge cases (castling, en passant) validated

## 🔧 Development

### Magic Bitboard Generation
Generate optimized magic numbers:
```bash
# Compile generator
gcc -O3 -march=native src/magic_bitboards.c -lpthread -o magic_generator

# Generate new magic constants (30+ seconds)
./magic_generator
```

### Performance Profiling
```bash
# Build with debug symbols
make clean && make DEBUG=1

# Profile with specific tests
./out/chess --perft 6
```

## 🎮 Chess Features

### Fully Implemented Rules
- ✅ **Piece Movement**: All standard piece movements
- ✅ **Castling**: Kingside and queenside, with proper restrictions
- ✅ **En Passant**: Pawn captures with position validation
- ✅ **Promotions**: Pawn promotion to any piece type
- ✅ **King Safety**: Check detection and legal move filtering
- ✅ **Turn Management**: Proper turn switching and game state

### Move Generation Types
- **MOVEGEN_ALL**: Complete legal move set
- **MOVEGEN_CAPTURES**: Capture moves only
- **MOVEGEN_QUIET**: Non-capture moves only

## 🔍 Technical Highlights

### Magic Bitboards
- **Concept**: Minimal perfect hashing for sliding piece attacks
- **Performance**: O(1) attack generation vs O(n) traditional methods
- **Implementation**: Hand-optimized magic constants with collision-free mapping
- **Memory**: Compact lookup tables with cache-friendly access patterns



## 📈 Performance Analysis

```
Perft Results (Starting Position):
Depth 1:        20 nodes |    2.50M nodes/sec
Depth 2:       400 nodes |   80.10M nodes/sec  
Depth 3:     8,902 nodes |   80.80M nodes/sec
Depth 4:   197,281 nodes |   81.72M nodes/sec
Depth 5: 4,865,609 nodes |   80.68M nodes/sec
```

