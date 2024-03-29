cmake_minimum_required(VERSION 3.13)
project(final_project C)

set(CMAKE_C_STANDARD 90)

add_executable(final_project
        main.c
        MemoryError.c
        MemoryError.h
        GameManager.h
        GameManager.c
        io/Parser.h
        io/Parser.c
        io/Printer.h
        io/Printer.c
        logic/backtracking.h
        logic/backtracking.c
        logic/ILP.h
        logic/ILP.c
        io/Command.c
        io/Command.h
        io/validators.h
        io/validators.c
        components/Game.c
        components/Game.h
        components/StatesList.c
        components/StatesList.h
        components/Board.c
        components/Board.h
        components/Move.c
        components/Move.h
        components/List.c
        components/List.h
        components/Stack.h
        components/Stack.c
        io/Serializer.c
        io/Serializer.h
        io/Error.c
        io/Error.h
        logic/actions.c
        logic/actions.h
        logic/LP.c
        logic/LP.h
        logic/ILP.c
        logic/ILP.h
        logic/solver.c
        logic/solver.h
        logic/random.c
        logic/random.h)