# ♟️ Chess Engine & Utilities

A modular, bitboard-based chess engine written in C, featuring efficient move generation, a simple GUI, and performance testing tools.

## 🚀 Features

- **Bitboard Representation**: Utilizes 64-bit bitboards for fast and memory-efficient board representation.
- **Comprehensive Move Generation**: Supports all standard chess moves, including:
  - Castling
  - En passant
  - Pawn promotions
- **Move Application**: Accurately applies moves, updating game state with respect to castling rights, en passant targets, and promotions.
- **Simple GUI**: Provides a basic graphical interface for rendering the board and handling user input.
- **Utility Libraries**: Includes array utilities for general-purpose array handling and timer utilities for performance measurement.
- **Performance Testing**: Features a perft (performance test) utility to validate move generation correctness and benchmark performance.

## 📁 Repository Structure

```plaintext
include/                    # Public headers
├── chess_board.h           # Board and move structures
├── chess_bitboard.h        # Low-level bitboard helpers
├── chess_gui.h             # GUI interface
├── move_generation.h       # Move generation functions
└── ...                     # Additional headers

src/                        # Source files
├── chess_board.c           # Board and move implementations
├── chess_bitboard.c        # Bitboard helper implementations
├── chess_gui.c             # GUI implementations
├── move_generation.c       # Move generation implementations
└── ...                     # Additional source files

images/                     # Images for documentation and GUI

makefile                    # Build configuration
README.md                   # Project documentation
```

## 🛠️ Build Instructions

Ensure you have a C compiler (e.g., `gcc`) installed. Then, build the project using the provided `makefile`:

```bash
make
```

This will compile the source files and generate the executable binaries.

## 🧪 Running Perft Tests

To validate move generation and benchmark performance, use the perft test utility.

The `run_perft_tests_up_to` function allows you to run perft tests up to a specified depth:

```c
void run_perft_tests_up_to(int max_depth);
```

For example, to run tests up to depth 5:

```c
run_perft_tests_up_to(5);
```

This function will output the number of nodes generated at each depth, along with timing and nodes-per-second metrics.

## 🖼️ GUI Usage

The project includes a simple GUI for visualizing the chessboard and interacting with the engine.

To launch the GUI:

```bash
./chess
```

This will open a window displaying the current board state, allowing you to make moves by clicking.

## 🤝 Contributing

Contributions are welcome! If you'd like to contribute to this project, please fork the repository and submit a pull request.
