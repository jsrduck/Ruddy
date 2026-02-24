# Ruddy
A strong, statically-typed programming language

Prereqs:

1. Install Python version 2.7 (not version 3) from https://www.python.org/
2. Download Quex from http://quex.sourceforge.net/ (currently using Quex version 0.65.10)
3. Make sure you have QUEX_PATH environment variable pointing to the location where you installed Quex
4. Install the following dependencies via vcpkg (https://github.com/Microsoft/vcpkg).
   * llvm:x64-windows-static (version 6.0.0-1)
   * boost-property-tree:x64-windows-static
   * boost-asio:x64-windows-static
   * libarchive:x64-windows-static
5. Because of a limitation in Visual Studio, you may have to run "build" 4 times the fisrt time (there are 4 custom build steps that VS won't run in sequence)

# 2026 Design Retrospective
This was a personal project intended as a learning exercise rather than a production-ready compiler. Fast iteration and exploration were more important to my goals than performance or long-term maintainability. I was primarily interested in exploring language semantics, type systems, and LLVM integration, rather than solving a specific gap in existing languages. The influence of C++ is obvious, though I also drew inspiration from Rust and Python.

Some reflections with the benefit of time:

* I used shared_ptr pervasively in the AST to simplify ownership during rapid iteration. Today I would prefer explicit ownership via unique_ptr or, if I were feeling more ambitious, an arena allocator to make lifetime semantics clear and improve locality and performance.
* I bundled llc directly in the compiler because I wanted a single standalone artifact. In retrospect, I would emit LLVM IR and rely on the standard toolchain instead of blurring the boundary between compiler and backend.
* I pushed into garbage collection because it was more intellectually interesting than reference counting. However, LLVM’s support for GC and stackmaps was (perhaps still is?) limited, and completing a production-quality design was beyond what I could reasonably tackle as a side project.
* I would intern types to make type equality comparisons cheap and canonical.
* My import libraries (“ribs”) would use a well-defined schema (maybe protobuf) rather than Boost serialization.
* I discovered that trying to unify value semantics (RAII) and reference semantics (GC-managed) under a single object model introduces deep tensions: what does this mean for a method that may be invoked on either a value-allocated or GC-allocated instance? Do I need two versions of every instance method? How do I reconcile LLVM's GC expecting a distinct address space for reference pointers with the fact that any instance method may or may not be in that address space? As a learning exercise, it was a success as it clarified why many production languages make sharper distinctions.

Overall, this project achieved its goal: it deepened my understanding of compiler architecture, object models, and the practical constraints imposed by real toolchains.

Thanks,
Jordan
