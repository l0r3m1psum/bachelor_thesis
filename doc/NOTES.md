<style type="text/css">
  @media (prefers-color-scheme: dark) {
    html {
      background-color: #1E1F21;
      color: #EEEFF1;
    }
    a {
      color: #EEEFF1;
    }
    a:visited {
      color: #EEEFF1;
    }
    blockquote p: {
      color: #606060;
    }
    hr {
      background-color: #EEEFF1;
    }
  }
</style>
# Libraries
  * [Google benchmark](https://github.com/google/benchmark)
    - [Chandler Carruth "Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!"](https://www.youtube.com/watch?v=nXaxk27zwlk)
    - [Chandler Carruth “Going Nowhere Faster”](https://www.youtube.com/watch?v=2EWejmkKlxs)
    - [How to use Google Benchmark for C++ programs](https://www.youtube.com/watch?v=9VKR8u9odrA)
  * [Google test](https://github.com/google/googletest)
    - [primer](https://github.com/google/googletest/blob/master/docs/primer.md)
  * [`syslog(3)`](https://pubs.opengroup.org/onlinepubs/9699919799/)
  * [SIMDJSON](https://github.com/simdjson/simdjson/)
  * [TCMalloc](https://github.com/google/tcmalloc)
  * [jemalloc](http://jemalloc.net)
  * [Sleef](https://sleef.org)

# Tools
  * [`perf(1)`](https://perf.wiki.kernel.org/index.php/Main_Page)
    - [Brendan Gregg](http://www.brendangregg.com/perf.html)
  * [`valgrind(1)`](https://www.valgrind.org)
  * [`git(1)`](https://git-scm.com)
  * [`make(1)`](https://pubs.opengroup.org/onlinepubs/9699919799/)
  * [`clang(1)`](https://clang.llvm.org)
    - [sanitizers](https://clang.llvm.org/docs/UsersManual.html#id39)
    - `-Wall`, `-Wextra`, `-Werror`, `-pedantic-error`
    - `-fno-exception`, `-fno-rtti`
    - [Extra Tools](https://clang.llvm.org/extra/index.html)
      - [`clang-tidy(1)`](https://clang.llvm.org/extra/clang-tidy/)
      - `clang-format(1)`
      - `clang-doc(1)` vs. `doxygen(1)`
  * `pmap(1)` & `vmmap(1)`
  * `readelf(1)` & `objdump(1)`
  * [Flame Graphs](http://www.brendangregg.com/flamegraphs.html)
  * [`gnuplot(1)`](http://gnuplot.info)
  * [Compiler Explorer](https://godbolt.org)
  * [`coz`](https://github.com/plasma-umass/coz)
  * [`stabilizer`](https://emeryberger.com/research/stabilizer/)
  * [Halide](https://halide-lang.org)
  * [coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)
    (for viewing code coverage by tests)
  * [`gflags`](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/gflags)
    (windows only)
  * [speedscope](https://www.speedscope.app)
  * [`-finstrument-functions`](https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-finstrument-functions)
    ```
    void __cyg_profile_func_enter(void *this_func, void *call_site) __attribute__ ((no_instrument_function));
    void __cyg_profile_func_exit(void *this_func, void *call_site) __attribute__ ((no_instrument_function));
    ```
    [Visualize function calls with Graphviz](https://web.archive.org/web/20130528172555/http://www.ibm.com/developerworks/library/l-graphvis/)
    [Trace and profile function calls with GCC](https://balau82.wordpress.com/2010/10/06/trace-and-profile-function-calls-with-gcc/)
    [How to profile your own function calls in C? (instrument your code!)](https://www.youtube.com/watch?v=M6RCUiZzl8Y)

# Misc
  * [Top 10 C++ Resources You MUST Know About!](https://www.youtube.com/watch?v=eSDVVrjFh54)
  * [Rust-style futures in C](https://axelf.nu/2020/08/24/rust-style-futures-in-c.html)

# Resources
  * Websites
    - [cppreferences](https://en.cppreference.com)
    - [Awesome Modern C++](https://awesomecpp.com)
  * People
    - [Jason Turner](https://twitter.com/lefticus)
    - [Coffe before arch](https://www.youtube.com/c/CoffeeBeforeArch)
    - [Creel](https://www.youtube.com/c/WhatsACreel)
    - [Brendan Gregg](https://www.brendangregg.com)
  * Data Oriented Design
    - [Data-Oriented Design and C++](https://www.youtube.com/watch?v=rX0ItVEVjHc)
    - [OOP Is Dead, Long Live Data-oriented Design](https://www.youtube.com/watch?v=yy8jQgmhbAU)
    - [C++ Crash Course: Data Oriented Design](https://www.youtube.com/watch?v=XpYORLssFW8)
    - [Data oriented game design](https://www.youtube.com/watch?v=GY9RytdA1mA)
    - [Building a Data-Oriented Future - Mike Acton](https://www.youtube.com/watch?v=u8B3j8rqYMw)
  * SWAR
    - [Using Integers as Arrays of Bitfields a.k.a. SWAR Techniques](https://www.youtube.com/watch?v=H-bm71KmYTE)
    - [The Aggregate Magic Algorithms](http://aggregate.org/MAGIC/)
    - [SWAR-Popcount](https://www.chessprogramming.org/Population_Count)
    - [A SWAR Algorithm for Popcount](https://www.playingwithpointers.com/blog/swar.html)
  * [What Every Programmer Should Know About Memory](https://akkadia.org/drepper/cpumemory.pdf)
  * [Pointer tricks](https://www.quora.com/In-C-or-C++-what-are-your-favorite-pointer-tricks)
  * Strict Aliasing
    - [strict aliasing 1](https://accu.org/journals/overload/28/160/anonymous/)
    - [strict aliasing 2](https://blog.regehr.org/archives/1307)

# Techniques
  * Floating point precision (multiply small number first then big numbers (non
  commutative operations))
  * Branchless programming
  * SIMD
  * Multi-threading
    - atomic
    - lock free (compare and exchange)
    - locks
  * Data Oriented Design (Structure Of Array)
  * Memory allignment
  * Memory order (relaxed acquired consumed)
  * Custom memory allocators
    - [gingerbill guide](http://www.gingerbill.org/series/memory-allocation-strategies/)
    - [always bump downwards](https://fitzgeraldnick.com/2019/11/01/always-bump-downwards.html)
  * SWAR (SIMD within a register)
  * Avoiding overflows
    - average [list of float](https://stackoverflow.com/a/1934266), [2 int](https://stackoverflow.com/questions/19106350/explanation-of-the-safe-average-of-two-numbers)
  * https://kholdstare.github.io/technical/2020/05/26/faster-integer-parsing.html

# Instructions
  * `__asm__("int3");` software interrupt for a breakpoint
  * `uint64_t __rdpmc(counter)` Read Performance-Monitoring Counters
  * `uint64_t __rdtsc(void)` Read Time-Stamp Counter

# Cool C stuff (TODO: INTEGRATE WITH THE REST)

## Cool C libs

[Eskil Steenberg](http://gamepipeline.org/index.html)
[raylib](https://www.raylib.com/index.html)
[sokol](https://github.com/floooh/sokol)
[stb](http://github.com/nothings/stb) ([author](http://nothings.org))

## Cool C comunities

[Handmade Network](https://handmade.network)
[C faq](http://c-faq.com)

## Cool C arcicles

[How to C in 2016](https://matt.sh/howto-c)
[So You Think You Can const](https://matt.sh/sytycc)
[Modern C for C++ Peeps](https://floooh.github.io/2019/09/27/modern-c-for-cpp-peeps.html)
[Centralized Memory Management](https://sasluca.github.io/cmm)

## Cool C talks

[Advice for Writing Small Programs in C](https://www.youtube.com/watch?v=eAhWIO1Ra6M)
[How I program C](https://www.youtube.com/watch?v=443UNeGrFoM)
  * talks about iterating over an array with a pointer
[Modern C and What We Can Learn From It](https://www.youtube.com/watch?v=QpAhX-gsHMs)
  * talks about the defer macro

## Cool C tecniques

  * [branchless programming](https://www.youtube.com/watch?v=bVJ-mWWL7cE)
  * double buffer tecnique
  * the struct trick (lo usa anche malloc)
  * [Flexible array member](https://en.wikipedia.org/wiki/Flexible_array_member)
  * [Stride of an array](https://en.wikipedia.org/wiki/Stride_of_an_array)
  * array iteration with pointer
  * goto end of unction for error
  * [int overflows](https://stackoverflow.com/a/1514309)
  * [X macros](https://en.wikipedia.org/wiki/X_Macro)

