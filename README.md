# clang2json

Dump type information from C headers and source files to line delimited json via libclang.

# Why?

For easy machine consumption in any language/program that understands JSON. Especially intended for bindings generators.

## Usage

```
[jared:~/projects/clang2json] ./build/clang2json
Usage
  clang2json [CLANG_OPTIONS]... FILE

Options
  CLANG_OPTIONS
    Same options you would pass to clang to compile this file.
    Ex: `-x c++` will tell clang this is a c++ file, not c.
    See `clang --help` or `man clang` for more information.

  FILE
    The file to convert.

Example
  # Analyze multiple files, appending the declarations into a single file.
  # This can (and probably should) be scripted if a large number of files are needed.
  clang2json -x c++ -I apr-1.5.1 apr-1.5.1/apr.h > declations.json
  clang2json -x c++ -I apr-1.5.1 apr-1.5.1/apr_file_io.h >> declarations.json
```

## Example

The result of running clang2json on the Apache APR library: [apr.json](https://github.com/jbreeden/clang2json/blob/master/apr.json)

## Spec

Just loook at the use of the JSON_* macros and you'll see exactly what is output for all of the various "entities" (structs, classes, parameters, etc.). All of the source is in a single file, and outputting JSON is pretty much the only thing it does, so it should be easy to thumb through.

## Cross Referencing

Most entities are uniquely identified by their USR (Unified Symbol Resolution). These USRs are used to specify references
between related entities.

For example, a `FieldDecl` will have a `"member_of"` property containing its parent `StructDecl`s USR. Similarly, a `ParmDecl` has a `"function"` member containing the USR of the function it belongs to. These USR references can
be used to construct a graph of the entities, like an AST of all type declarations from the source library.
