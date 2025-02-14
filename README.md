# 动机

之前看到过一个说法，讲Cpp的模板是图灵完备的。怎么证明呢？最直接的办法就是写一个图灵机模拟。

**Cpp模板元编程可以看做是一种函数式编程，因此可以实现图灵机**

于是翻出来计算原理的笔记，复习了一下图灵机是怎么回事。一个图灵机可以拆分成如下几个部分：

- 字母表
- 纸带（Tape）和读写头（Cursor）
- 转换函数（Delta）

# 实现字母表

这一块很容易，模板元编程操作的是类型，那么一个类型就是一个字母。
为了后续验证顺利，为了后续验证顺利，写了一个对整数的Wrapper。并用wrapper类包裹了三个字母：

- 0（Nil）
- 1 （Uno)
- 空格（Blank）

```cpp
template<std::size_t sz>
struct SizeTWrapper {
    static constexpr auto value = sz;
};

struct BlankSymbol {
};

using Uno = SizeTWrapper<1>;
using Nil = SizeTWrapper<0>;
```



# 实现纸带+读写头

## 实现思路

如果正常实现纸带非常简单，需要：

- 一个数组，存储纸带上的字母/元素
- 一个int，存储光标的位置

## 模板实现

模板元编程实现纸带：

- 可变参数模板代表纸带上的元素，为了简便，默认元素个数>=1
- 另一个```std::size_t```模板参数记录光标位置

### 实现下标访问

需要用模板实现形如 ```arr[1]```这样的访问。C++的feature并不提供模板参数包的下标访问，那么只能通过最简单的方式：递归。（有FP那个味了）
对idx=0的情况特化，其余的递归处理

```cpp
template<std::size_t N, typename T0, typename ...Ts>
struct Nth {
    using type = typename Nth<N - 1, Ts...>::type;
};

template<typename T0, typename ...Ts>
struct Nth<0, T0, Ts...> {
    using type = T0;
};
```

### 纸带的实现

一个```std::size_t``` + 可变模板参数实现纸带。额外维护了几个编译期常量告知纸带的长、光标位置、光标指向的符号和任意一处的符号。

```cpp
template<std::size_t N, typename T0, typename... Ts>
struct Tape {
    static constexpr auto length = 1 + sizeof...(Ts);
    static constexpr auto pos = N;
    using TapeSymbol = typename Nth<N, T0, Ts...>::type;
    template<std::size_t idx>
    using Symbol = typename Nth<idx, T0, Ts...>::type; // tape[idx]
};
```



## 实现纸带的动作

这里的纸带的动作分为两种：

1. 写一个字符
2. 光标向左向右移动一格

### 实现写字符

这一节基本就是FP了。首先我们要有一个判定函数：

```python
predicate = lambda old_symbol, new_symbol: return old_symbol if pos != cursor else new_symbol
```

然后对原有的纸带做map：

```python
new_tape = map(old_tape, predicate)
```

#### 实现Predicate

用模板的两个特化实现Predicate

```cpp
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
```

#### 通过遍历参数列表实现PutSymbol

C++提供了参数包展开的语法(```...```)。只要实现一个整数参数包，例如```Class<0, 1, 2, 3>```，对其展开，就可以实现编译期的遍历。

标准库内实际上提供了```std::integer_sequence```，不过当时并不知道。自己实现了一个```Seq```类型和```SeqMaker```类型。更多的函数式编程。

其中```SeqMaker<N>::Seq```可以方便实现实现参数是0, 1, ... (N-1)的模板类。

```cpp
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
    using Seq = typename Append<SizeTWrapper<seq_len - 1>, typename SeqMaker<seq_len - 1>::Seq>::type;
};

template<>
struct SeqMaker<1> {
    using Seq = Seq<SizeTWrapper<0>>;
};
```

之后可以实现PutSymbol（写入一个符号）了。先实现一个helper类，用于生成（在Tape任意位置写入任意一个符号）之后的Tape。

```cpp
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


```

简化一下，因为我们只会在光标所在的位置写东西，而光标位置是已知的，而整数序列可以通过```SeqMaker```快速获得：

```cpp
template<typename WrittenSymbol, typename TapeType>
struct PutSymbolAt {
    using TapeAfter = typename PutSymbolHelper<SizeTWrapper<TapeType::pos>, WrittenSymbol, TapeType, typename SeqMaker<TapeType::length>::Seq>::TapeAfter;
};
```

### 实现光标的动作

光标动作正常只需要把```Tape::pos```做变换即可。但需要注意特殊边界情况。如果移动出了边界，需要给纸带添加一个空格。

用一个helper类获取纸带的构成，然后组建新的纸带：

```cpp
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
```

左移也是同理：

```cpp
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
```

```MoveLeft``` 和 ```MoveRight```两个类型会指导图灵机的动作。

# 实现图灵机和Delta函数

## 实现图灵机

图灵机由两部分组成

- 纸带
- 状态

代码直接写就好：

```cpp
template<typename StateType, typename TapeType>
struct TuringMachine {
    using State = StateType;
    using Tape = TapeType;
};
```

此外要添加一个类型作为停机（Halt）状态：

```cpp
struct HaltState {
};
```

纸带+状态 = 图灵机的格局（configuration）

## 实现Delta函数

Delta函数用于转换图灵机的状态，其输入输出分别是

- 当前纸带
- 当前图灵机的状态
    输出：
- 写入的字符
- 光标移动的方向
- 下一个状态

这里为了实现方便（没有太多嵌套的```::```）加入了一个独立的输入，即当前读到的符号，即使这个输入可以从纸带获得。
代码是一个example，Movement复用之前的类型。

```cpp
template<typename Tape, typename CurState, typename CurSymbol>
struct Delta {
    using WrittenSymbol = void;
    template<class TapeType>
    using Movement = void; // struct MoveLeft or struct MoveRight
    using NextState = void; 
};
```

## 实现Step

这一步是重中之重了。我们要通过Delta提供的信息，运行一步图灵机。

图灵机能否运行的先决条件是没有停机。所以```Step```分两个特化：

```cpp
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
```

分开来说：

- 用Delta的模板匹配当作“在当前纸带上运行Delta函数”，匹配类型的输出就是接下来图灵机的运行方向DeltaType
- DeltaType中的写入字符，配合之前实现的字符写入模板，获得一个新纸带
- DeltaType中的移动方向信息，可以在新纸带上移动光标
- 新光标位置 + 新纸带 + 新状态 = 新的图灵机格局。Step完成。

## 实现图灵机的运行

从初始的格局出发，递归运行Step，直到状态为halt。

```cpp
template<typename TuringMachineType>
struct Run {
    using type = typename Step<TuringMachineType, std::is_same_v<typename TuringMachineType::State, HaltState>>::type;
};
```



# 验证

写了三个Delta函数，用于寻找二进制串中的第一个1（Uno）。这里可以进一步看出Delta函数是如何工作的

```cpp
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
```

测试函数：

```cpp
int main() {
    using T = Tape<0, Nil, Nil, Nil, Uno, Nil, Uno>; // [0*, 0, 0, 1, 0]
    using TM = TuringMachine<UnoState, T>;
    using Result = Run<TM>::type;
    std::cout << Result::Tape::length << std::endl;
    std::cout << Result::Tape::pos << std::endl;
    std::cout << Result::Tape::TapeSymbol::value << std::endl;
}
```



使用compiler-explorer汇编代码，发现确实是编译期生成的，除了```std::cout```不涉及到任何runtime：

```assembly
main:
        push    rbp
        mov     rbp, rsp
        mov     esi, 5    # length of tape
        mov     edi, OFFSET FLAT:std::cout
        call    std::ostream::operator<<(unsigned long)
        mov     esi, OFFSET FLAT:std::basic_ostream<char, std::char_traits<char>>& std::endl<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&)
        mov     rdi, rax
        call    std::ostream::operator<<(std::ostream& (*)(std::ostream&))
        mov     esi, 3    # cursor pos
        mov     edi, OFFSET FLAT:std::cout
        call    std::ostream::operator<<(unsigned long)
        mov     esi, OFFSET FLAT:std::basic_ostream<char, std::char_traits<char>>& std::endl<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&)
        mov     rdi, rax
        call    std::ostream::operator<<(std::ostream& (*)(std::ostream&))
        mov     esi, 1   # tape symbol value
        mov     edi, OFFSET FLAT:std::cout
        call    std::ostream::operator<<(unsigned long)
        mov     esi, OFFSET FLAT:std::basic_ostream<char, std::char_traits<char>>& std::endl<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&)
        mov     rdi, rax
        call    std::ostream::operator<<(std::ostream& (*)(std::ostream&))
        mov     eax, 0
        pop     rbp
        ret
```





或者使用cpp-insight做模板展开，也可以发现最后的模板是对的：

```cpp
// 省略之前的1021行展开代码
int main()
{
  using T = Tape<0, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> >; // [0*, 0, 0, 1, 0]
  using TM = TuringMachine<SizeTWrapper<1>, Tape<0, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> > >;
  using Result = Step<TuringMachine<HaltState, Tape<3, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> > >, true>::TuringMachine<HaltState, Tape<3, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> > >; // 这里可以看到停机了，并且cursor在第一个1的位置
  std::cout.operator<<(Tape<3, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> >::length).operator<<(std::endl);
  std::cout.operator<<(Tape<3, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<0>, SizeTWrapper<1>, SizeTWrapper<0> >::pos).operator<<(std::endl);
  std::cout.operator<<(SizeTWrapper<1>::value).operator<<(std::endl);
  return 0;
}
```



# 总结

用过程式编程语言，使用函数式编程的方法，在编译期干了本该是运行期干的活。

不过这也可以证明，cpp模板确实是图灵完备的，也就是说，C++模板理论上能写出**任何**程序。
