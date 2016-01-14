# clang2json
Dump type information from C headers to line delimited json via libclang

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

## Features
- Reads declarations of classes, structs, fields, functions, parameters, macros, typedefs, etc., from header files.
- Prints the declaration information complete with type data as line-delimited json.
- The output is suitable for use in code generators (say for [generating mruby bindings](https://github.com/jbreeden/mruby-bindings))
  or whatever else you may need the type information from a library for.

## Output Format
Not _stictly_ defined at the moment. Have a look at [apr.json](https://github.com/jbreeden/clang2json/blob/master/apr.json) for an example.
If this doesn't do it for you... the code is short and simple. Just loook at the use of the JSON_* macros and you'll see exactly
what is output for all of the various "entities" (structs, classes, parameters...).

## Cross Referencing
Most entities are uniquely identified by their 'usr' (Unified Symbol Resolution). These usr's are used to specify references
between related entities.

For example, a `FieldDecl` will have a `"member_of"` property containing its parent `StructDecl`s usr. Similarly, a `ParmDecl` has a `"function"` member containing the usr of the function it belongs to. These usr references can
be used to construct a graph of the entities, like an AST of just type declarations.
