//
// Created by 刘宇轩 on 2023/11/28.
//

#ifndef MYTUPLE_RULES_H
#define MYTUPLE_RULES_H

#include "TuringMachine.h"

/* These are the rules for a machine that finds the first Uno of a string*/
template<typename Tape>
struct Delta<Tape, UnoState, Nil> {
    using WrittenSymbol = Nil;
    template<class TapeType>
    using Movement = MoveLeft<TapeType>;
    using NextState = UnoState;
};

template<typename Tape>
struct Delta<Tape, UnoState, Uno> {
    using WrittenSymbol = Uno;
    template<class TapeType>
    using Movement = MoveLeft<TapeType>;
    using NextState = NilState;
};

template<typename Tape>
struct Delta<Tape, NilState, BlankSymbol> {
    using WrittenSymbol = BlankSymbol;
    template<class TapeType>
    using Movement = MoveRight<TapeType>;
    using NextState = HaltState;
};

template<typename Tape>
struct Delta<Tape, NilState, Uno> {
    using WrittenSymbol = Uno;
    template<class TapeType>
    using Movement = MoveRight<TapeType>;
    using NextState = HaltState;
};

template<typename Tape>
struct Delta<Tape, NilState, Nil> {
    using WrittenSymbol = Nil;
    template<class TapeType>
    using Movement = MoveRight<TapeType>;
    using NextState = HaltState;
};

#endif //MYTUPLE_RULES_H
