# clang2json
Dump type information from C headers to line delimited json via libclang

## Features
- Reads declarations of classes, structs, fields, functions, parameters, macros, typedefs, etc., from header files.
- Prints the declaration information complete with type data as line-delimited json.
- The output is suitable for use in code generators (say for [generating mruby bindings](https://github.com/jbreeden/mruby-bindings))
  or whatever else you may need the type information of a library for.
  
## Output Format
Not _stictly_ defined at the moment. Have a look at [libapr.json](https://github.com/jbreeden/clang2json/blob/master/libapr.json) for an example.
If this doesn't do it for you... the code is short and simple. Just loook at the use of the JSON_* macros and you'll see exactly
what is output for all of the various "entities" (structs, classes, parameters...).

## Cross Referencing
Most entities are uniquely identified by their 'usr' (Unified Symbol Resolution). These usr's are used to specify references
between related entities (Notable exception: parameters don't yet have this cross reference. They simply appear after the 
function they belong to. This will be fixed at some point, but for the time being it suffices to track the lasted encountered 
function and associate any paramters with that function.).

For example, a FieldDecl will have a 'member_of' property containing its parent StructDecl's usr. These usr references can 
be used to construct a graph of the entities, similar to an AST.

The main differences between the data provided by clang2json (once graphed) and an actual AST are that
- clang2json only concerns itself with declarations (what are the types? what are their functions? what parameters do they take?)
  + This does not include information about function bodies, or other implementation details.
  + Basically, this is anything useful to create a binding to the C library.
- An AST would have multiple "declarations" of the same types.
  + Ex: A struct my be represented once when actually defined, again for any forward-declarations, and yet again in any typedefs.
  + clang2json only cares about the "canonical" definition of the types, and only outputs this data once.
