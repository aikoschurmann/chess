# Project Name

Chess Engine & Utilities

## Overview

This repository contains a C-based chess engine along with supporting utilities and a simple GUI. The core is a bitboard-based move generator and executor, with features for:

* **Board representation** using 64-bit bitboards
* **Move generation** for all piece types (pawn, knight, bishop, rook, queen, king) including special moves (castling, en passant, promotions)
* **Move application** with castling rights, en passant, and promotions
* **Simple GUI** for rendering the board and handling user input
* **Array utilities** (array operations, broadcasting, allocations) for general-purpose array handling
* **Timer** utilities for performance measurement

## Repository Structure

```
include/                    # Public headers
  ├─ chess_board.h          # Board and move structs
  ├─ chess_bitboard.h       # Low-level bitboard helpers
  ├─ chess_gui.h            # GUI interface
  ├─ move_generation.h      # Move generation 
  └─ ...
src/                        # Source files
  ├─ chess_board.c          # Board initialization & applying of moves
  ├─ move_generation.c      # Move generation implementations
  ├─ chess_bitboard.c       # Bitboard primitives
  ├─ chess_gui.c            # GUI implementation
  ├─ gui.c                  # SDL low level implementation
  ├─ main.c                 # CLI front-end and entry point
  └─ ...

README.md                   # This file
...
```

## Build & Installation

1. **Prerequisites:**

   * GCC or Clang
   * Make

2. **Build:**

   ```bash
   make
   ```

3. **Run:**

    ```bash
     make run
     ```


## Usage

* **Interactive Play:**

  * Use mouse to select and move pieces in the GUI.
  * Press b to enable the bitboard masks, use arrows to change the piece



## Testing

* To be implemented

## Future Work

* Optimize move generation with magic bitboards
* Implement min-max etc