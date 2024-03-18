#include <iostream>
#include "TuringMachine.h"
#include "rules.h"


int main() {
    using T = Tape<0, Nil, Nil, Nil, Uno, Nil, Uno>; // [0*, 0, 0, 1, 0, 1]
    using TM = TuringMachine<UnoState, T>;
    using Result = Run<TM>::type;
    std::cout << Result::Tape::length << std::endl;
    std::cout << Result::Tape::pos << std::endl;
    std::cout << Result::Tape::TapeSymbol::value << std::endl;
}
