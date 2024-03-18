//
// Created by 刘宇轩 on 2023/11/28.
//

#ifndef MYTUPLE_TURINGMACHINE_H
#define MYTUPLE_TURINGMACHINE_H

#include <cstddef>

template<std::size_t sz>
struct SizeTWrapper {
    static constexpr auto value = sz;
};

struct BlankSymbol {
};

struct HaltState {
};

using Uno = SizeTWrapper<1>;
using Nil = SizeTWrapper<0>;
using UnoState = Uno;
using NilState = Nil;

template<typename ...Numbers>
struct Seq;

template<typename, typename ...>
struct Append;

template<typename Item, typename ...Numbers>
struct Append<Item, Seq<Numbers...>> {
    using type = Seq<Numbers..., Item>;
};

template<std::size_t seq_len>
struct SeqMaker {
//    using Seq = typename SeqMaker<seq_len - 1>::Seq::template append<seq_len>;
    using Seq = typename Append<SizeTWrapper<seq_len - 1>, typename SeqMaker<seq_len - 1>::Seq>::type;
};

template<>
struct SeqMaker<1> {
    using Seq = Seq<SizeTWrapper<0>>;
};


template<typename T0, typename T1, bool>
struct Predicate;

template<typename T0, typename T1>
struct Predicate<T0, T1, true> {
    using type = T1;
};

template<typename T0, typename T1>
struct Predicate<T0, T1, false> {
    using type = T0;
};


template<std::size_t N, typename T0, typename ...Ts>
struct Nth {
    using type = typename Nth<N - 1, Ts...>::type;
};

template<typename T0, typename ...Ts>
struct Nth<0, T0, Ts...> {
    using type = T0;
};

//template<typename T0>
//struct Nth<0, T0> {
//    using type = T0;
//};

template<std::size_t N, typename T0, typename... Ts>
struct Tape {
    static constexpr auto length = 1 + sizeof...(Ts);
    static constexpr auto pos = N;
    using TapeSymbol = typename Nth<N, T0, Ts...>::type;
    template<std::size_t idx>
    using Symbol = typename Nth<idx, T0, Ts...>::type;
};

template<typename, typename, typename, typename>
struct PutSymbolHelper;

template<typename Where, //where to write this symbol
        typename WrittenSymbol, typename TapeType,
        typename ...idx>
struct PutSymbolHelper<Where, WrittenSymbol, TapeType, Seq<idx...>> {
    using TapeAfter = Tape<TapeType::pos,
            typename Predicate<WrittenSymbol, typename TapeType::template Symbol<idx::value>, // choose new or old symbol
                    (idx::value != Where::value)>::type...>;
};

template<typename WrittenSymbol, typename TapeType>
struct PutSymbolAt {
    using TapeAfter = typename PutSymbolHelper<SizeTWrapper<TapeType::pos>, WrittenSymbol, TapeType, typename SeqMaker<TapeType::length>::Seq>::TapeAfter;
};


template<typename, typename>
struct WriteHelper;

template<std::size_t idx, typename WrittenType, typename ...Ts>
struct WriteHelper<Tape<idx, Ts...>, WrittenType> {
    template<std::size_t... seq>
    using TapeType = Tape<idx, Predicate<WrittenType, Ts, std::is_same<Ts, WrittenType>::value>...>;
};


template<typename WrittenSymbol, typename TapeType, bool is_same /* = true*/>
struct TapeAfterWriteHelper {
    using type = TapeType;
};

template<typename WrittenSymbol, typename TapeType>
struct TapeAfterWriteHelper<WrittenSymbol, TapeType, false> {
    using type = void;
};

template<bool is_end /* = true */, std::size_t N, typename ...Ts>
struct MoveRightHelper {
    using TapeAfter = Tape<0, BlankSymbol, Ts...>; // add a blank
};

template<std::size_t N, typename ...Ts>
struct MoveRightHelper<false, N, Ts...> {
    using TapeAfter = Tape<N - 1, Ts...>;
};


template<typename ...>
struct MoveRight;

template<std::size_t N, typename ...Ts>
struct MoveRight<Tape<N, Ts...>> {
    using TapeAfter = typename MoveRightHelper<N == 0, N, Ts...>::TapeAfter;
};

template<bool is_end /* = true */, std::size_t N, typename ...Ts>
struct MoveLeftHelper {
    using TapeAfter = Tape<N + 1, Ts..., BlankSymbol>;
};

template<std::size_t N, typename ...Ts>
struct MoveLeftHelper<false, N, Ts...> {
    using TapeAfter = Tape<N + 1, Ts...>;
};

template<typename ...>
struct MoveLeft;

template<std::size_t N, typename ...Ts>
struct MoveLeft<Tape<N, Ts...>> {
    using TapeAfter = typename MoveLeftHelper<(sizeof...(Ts) == N + 1), N, Ts...>::TapeAfter;
};


template<typename StateType, typename TapeType>
struct TuringMachine {
    using State = StateType;
    using Tape = TapeType;
};

template<typename Tape, typename CurState, typename CurSymbol>
struct Delta {
    using WrittenSymbol = void;
    template<class TapeType>
    using Movement = void;
    using NextState = void;
};


template<typename TuringMachineType, bool is_halt /* = true */>
struct Step {
    using type = TuringMachineType; // do nothing
};

template<typename TuringMachineType>
struct Step<TuringMachineType, false> {
    using DeltaType = Delta<typename TuringMachineType::Tape, typename TuringMachineType::State, typename TuringMachineType::Tape::TapeSymbol>;
    using TapeAfterWritten = typename PutSymbolAt<typename DeltaType::WrittenSymbol, typename TuringMachineType::Tape>::TapeAfter;
    using TapeAfterMove = typename DeltaType::template Movement<TapeAfterWritten>::TapeAfter;
    using NextTuringMachine = TuringMachine<typename DeltaType::NextState, TapeAfterMove>;
    using type = typename Step<NextTuringMachine, std::is_same_v<typename NextTuringMachine::State, HaltState>>::type; // new configuration
};

template<typename TuringMachineType>
struct Run {
    using type = typename Step<TuringMachineType, std::is_same_v<typename TuringMachineType::State, HaltState>>::type;
};


#endif //MYTUPLE_TURINGMACHINE_H
