#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <string>
#include <deque>
#include <algorithm>
#include "clang-c/Index.h"

using namespace std;

class Context;
class LocationDescription;

string to_s_and_dispose(CXString cxString);
CXChildVisitResult visit(CXCursor cursor, CXCursor parent, CXClientData data);
CXChildVisitResult visitTypedefDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitMacroDefinition(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitNamespace(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitFunctionDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitTypeDecl(string kind, CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitClassTemplate(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitUnionDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitEnumDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitEnumConstantDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitClassDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitStructDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitCXXBaseSpecifier(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitCXXMethod(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitParmDecl(CXCursor cursor, CXCursor parent, Context* context);
CXChildVisitResult visitFieldDecl(CXCursor cursor, CXCursor parent, Context* context);

class Context {
public:
   /*
    * TODO: Track current function and add USR reference to params
    */
   CXTranslationUnit translation_unit;
   deque<string> nested_namespaces;
   deque<string> nested_types;

   string current_namespace() {
      if (nested_namespaces.empty()) {
         return "";
      }

      return nested_namespaces.back();
   }

   string current_type() {
      if (nested_types.empty()) {
         return "";
      }

      return nested_types.back();
   }
};

class TypeData {
public:
   CXType type;
   string type_name;
   string type_usr;
   bool type_is_pointer;
   CXType pointee_type;
   string pointee_type_name;
   string pointee_type_usr;
   CXCursor canonical_cursor;

   TypeData(CXType type) {
      this->type = clang_getCanonicalType(type);
      canonical_cursor = clang_getTypeDeclaration(this->type);
      type_name = to_s_and_dispose(clang_getTypeSpelling(type));
      type_usr = to_s_and_dispose(clang_getCursorUSR(clang_getTypeDeclaration(type)));
      type_is_pointer = type.kind == CXType_Pointer;
      pointee_type_name = "";
      pointee_type_usr = "";
      if (type_is_pointer) {
         pointee_type = clang_getCanonicalType(clang_getPointeeType(type));
         pointee_type_name = to_s_and_dispose(clang_getTypeSpelling(pointee_type));
         pointee_type_usr = to_s_and_dispose(clang_getCursorUSR(clang_getTypeDeclaration(pointee_type)));
      }
   }
};

class LocationDescription {
public:
   const char * file;
   unsigned int line;
   unsigned int column;
   unsigned int offset;
   unsigned int length;

   LocationDescription(CXCursor cursor) {
      CXSourceLocation location = clang_getCursorLocation(cursor);
      CXFile cxFile;
      clang_getFileLocation(location, &cxFile, &line, &column, &offset);
      CXSourceRange extent = clang_getCursorExtent(cursor);
      CXSourceLocation endLocation = clang_getRangeEnd(extent);
      unsigned int endOffset;
      clang_getFileLocation(endLocation, NULL, NULL, NULL, &endOffset);
      length = endOffset - offset;

      cxsFile = clang_getFileName(cxFile);
      file = clang_getCString(cxsFile);
   }

   string read() {
      FILE* f = fopen(file, "r");
      char* buf = (char*)malloc(sizeof(char) * (length + 1));
      fseek(f, offset, SEEK_SET);
      fread(buf, length, 1, f);
      buf[length] = '\0';
      string result(buf, length);
      free(buf);
      return result;
   }

   ~LocationDescription() {
      clang_disposeString(cxsFile);
   }

private:
   CXString cxsFile;
};

// TODO: These would make better functions than macros...

#define START_JSON \
   do { \
      bool is_first_json_field = true; \
      cout << "{ "; \
      LocationDescription location(cursor); \
      JSON_STRING("file", location.file) \
      JSON_STRING("line", location.line) \

#define JSON_VALUE(k, v) \
      if (is_first_json_field) { \
         cout << "\"" << k << "\": " << v; \
         is_first_json_field = false; \
      } else { \
         cout << ", \"" << k << "\": " << v; \
      }

#define JSON_STRING(k, v) \
      JSON_VALUE(k, "\"" << v << "\"")

#define JSON_OBJECT(k)\
      if (is_first_json_field) { \
         cout << "\"" << k << "\"" << ": { ";\
      } else { \
         cout << ", \"" << k << "\"" << ": { ";\
      }\
      is_first_json_field = true;\

#define JSON_OBJECT_END\
      cout << "}";\
      is_first_json_field = false; \

#define JSON_TYPE_DATA(key, type_data) \
      JSON_OBJECT(key)\
         JSON_STRING("type_name", type_data.type_name)\
         JSON_STRING("type_usr", type_data.type_usr)\
         JSON_VALUE("type_is_pointer", (type_data.type_is_pointer ? "true" : "false"))\
         if (type_data.type_is_pointer) {\
            JSON_STRING("pointee_type_name", type_data.pointee_type_name)\
            JSON_STRING("pointee_type_usr", type_data.pointee_type_usr)\
         }\
      JSON_OBJECT_END

#define END_JSON \
      cout << " }" << endl; \
   } while (0);

string to_s_and_dispose(CXString cxString) {
   string result = clang_getCString(cxString);
   clang_disposeString(cxString);
   return result;
}

string getCursorUSRString(CXCursor cursor) {
   return to_s_and_dispose(clang_getCursorUSR(cursor));
}

CXChildVisitResult visitTypedefDecl(CXCursor cursor, CXCursor parent, Context* context) {
   string name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   TypeData type_data(clang_getCursorType(cursor));
   TypeData underlying_type_data(clang_getTypedefDeclUnderlyingType(cursor));

   START_JSON
      JSON_STRING("kind", "TypedefDecl")
      JSON_STRING("name", name)
      JSON_TYPE_DATA("type", type_data)
      JSON_TYPE_DATA("underlying_type", underlying_type_data)
   END_JSON

   return CXChildVisit_Recurse;
}

string expand_escapes(const char* src)
{
   char c;
   string result = "";

   while ((c = *(src++))) {
      switch (c) {
      case '\a':
         result += '\\';
         result += 'a';
         break;
      case '\b':
         result += '\\';
         result += 'b';
         break;
      case '\t':
         result += '\\';
         result += 't';
         break;
      case '\n':
         result += '\\';
         result += 'n';
         break;
      case '\v':
         result += '\\';
         result += 'v';
         break;
      case '\f':
         result += '\\';
         result += 'f';
         break;
      case '\r':
         result += '\\';
         result += 'r';
         break;
      case '\\':
         result += '\\';
         result += '\\';
         break;
      case '\"':
         result += '\\';
         result += '\"';
         break;
      default:
         result += c;
      }
   }
   return result;
}

CXChildVisitResult visitMacroDefinition(CXCursor cursor, CXCursor parent, Context* context) {
   string macro = to_s_and_dispose(clang_getCursorSpelling(cursor));
   LocationDescription location(cursor);

   START_JSON
      JSON_STRING("kind", "MacroDefinition")
      JSON_STRING("name", macro)
      JSON_STRING("text", expand_escapes(location.read().c_str()))
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visitNamespace(CXCursor cursor, CXCursor parent, Context* context) {
   string usr = getCursorUSRString(cursor);

   // TODO: Emit namespace object in json
   context->nested_namespaces.push_back(usr);
   clang_visitChildren(cursor, visit, context);
   context->nested_namespaces.pop_back();

   return CXChildVisit_Continue;
}

#include <vector>
CXChildVisitResult visitTypeDecl(string kind, CXCursor cursor, CXCursor parent, Context* context) {
   static vector<string> visited_usrs;

   string name;
   string usr;
   TypeData type_data(clang_getCursorType(cursor));
   if (cursor.kind == CXCursor_ClassTemplate) {
      // The type spelling is blank for templates, since they aren't a real type yet (I guess).
      //
      // The cursor spelling *usually* works for non-template types as well, except those that are anonymous.
      // For example, the following declaration would give an empty cursor spelling, but the correct type spelling:
      //  ```
      //  typedef struct {
      //     int some_int;
      //   } my_struct;
      //  ```
      // So, prefer the type spelling which works for typedefs and regular type declarations.
      name = to_s_and_dispose(clang_getCursorSpelling(cursor));
      usr = getCursorUSRString(cursor);
   }
   else {
      name = to_s_and_dispose(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(cursor))));
      usr = type_data.type_usr;
   }

   /* Types tend to be declared multiple times via forward declarations, typedefs, and actual definitions.
    * We don't want to print the type every time we see it. Rather, the first time we see it, we want
    * to get the canonical type (the actual definition), print the type information, and then ignore
    * it if we see it again.
    *
    * So, we store the usr's we've already seen and ignore them thereafter.
    *
    * Not that it is very common to have a type declaration or typedef and an actual type definition
    * immediately follow one another. To optimize for this case, we search the list of visited usr's
    * in reverse order.
    */
   for (vector<string>::reverse_iterator it = visited_usrs.rbegin(); it != visited_usrs.rend(); it++) {
      if (*it == usr) {
         return CXChildVisit_Continue;
      }
   }
   visited_usrs.push_back(usr);

   START_JSON
      JSON_STRING("kind", kind)
      JSON_STRING("name", name)
      JSON_STRING("namespace", context->current_namespace())
      JSON_STRING("usr", usr)
      if (context->current_type().size() > 0) {
         JSON_STRING("nested_in", context->current_type())
      }
      JSON_TYPE_DATA("type", type_data)
   END_JSON

   context->nested_types.push_back(usr);
   /* Use the canonical cursor so we can be sure to pick up the fields correctly,
    * otherwise we might recurse over a typedef like `typedef mystruct mystruct;`
    * and miss all the good bits. (recall that we'll only visit a type once: see above)
    */
   clang_visitChildren(type_data.canonical_cursor, visit, context);
   context->nested_types.pop_back();

   return CXChildVisit_Continue;
}

CXChildVisitResult visitClassTemplate(CXCursor cursor, CXCursor parent, Context* context) {
   return visitTypeDecl("ClassTemplate", cursor, parent, context);
}

CXChildVisitResult visitUnionDecl(CXCursor cursor, CXCursor parent, Context* context) {
   return visitTypeDecl("UnionDecl", cursor, parent, context);
}

CXChildVisitResult visitEnumDecl(CXCursor cursor, CXCursor parent, Context* context) {
   return visitTypeDecl("EnumDecl", cursor, parent, context);
}

CXChildVisitResult visitEnumConstantDecl(CXCursor cursor, CXCursor parent, Context* context) {
   string name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   long long value = clang_getEnumConstantDeclValue(cursor);
   string usr = getCursorUSRString(cursor);

   START_JSON
      JSON_STRING("kind", "EnumConstantDecl")
      JSON_STRING("name", name)
      JSON_VALUE("value", value)
      JSON_STRING("member_of", context->current_type())
      JSON_STRING("usr", usr)
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visitClassDecl(CXCursor cursor, CXCursor parent, Context* context) {
   return visitTypeDecl("ClassDecl", cursor, parent, context);
}

CXChildVisitResult visitStructDecl(CXCursor cursor, CXCursor parent, Context* context) {
   return visitTypeDecl("StructDecl", cursor, parent, context);
}

CXChildVisitResult visitCXXBaseSpecifier(CXCursor cursor, CXCursor parent, Context* context) {
   string type_name = to_s_and_dispose(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(cursor))));
   string parent_usr = getCursorUSRString(clang_getTypeDeclaration(clang_getCanonicalType(clang_getCursorType(cursor))));

   TypeData base_type(clang_getCursorType(cursor));

   START_JSON
      JSON_STRING("kind", "CXXBaseSpecifier")
      JSON_STRING("name", type_name)
      JSON_TYPE_DATA("base_type", base_type)
      JSON_STRING("child_usr", context->current_type())
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visitCXXMethod(CXCursor cursor, CXCursor parent, Context* context) {
   string method_name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(cursor);
   TypeData return_type_data(clang_getCursorResultType(cursor));
   string usr = getCursorUSRString(cursor);

   START_JSON
      JSON_STRING("kind", "CXXMethod")
      JSON_STRING("name", method_name)
      JSON_TYPE_DATA("return_type", return_type_data)
      switch (access) {
      case CX_CXXPrivate:
         JSON_STRING("access", "private")
         break;
      case CX_CXXPublic:
         JSON_STRING("access", "public")
         break;
      case CX_CXXProtected:
         JSON_STRING("access", "protected")
         break;
      case CX_CXXInvalidAccessSpecifier:
        JSON_STRING("access", "invalid")
        break;
      }
      if (clang_CXXMethod_isVirtual(cursor)) {
         JSON_VALUE("is_virtual", "true")
      }
      else {
         JSON_VALUE("is_virtual", "false")
      }
      JSON_STRING("member_of", context->current_type())
      JSON_STRING("usr", usr)
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visitFunctionDecl(CXCursor cursor, CXCursor parent, Context* context) {
   string name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   TypeData return_type_data(clang_getCursorResultType(cursor));
   string usr = getCursorUSRString(cursor);

   START_JSON
      JSON_STRING("kind", "FunctionDecl")
      JSON_STRING("name", name)
      JSON_TYPE_DATA("return_type", return_type_data)
      JSON_STRING("namespace", context->current_namespace())
      JSON_STRING("usr", usr)
      END_JSON

      return CXChildVisit_Recurse;
}

CXChildVisitResult visitParmDecl(CXCursor cursor, CXCursor parent, Context* context) {
   string param_name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   CXType type = clang_getCanonicalType(clang_getCursorType(cursor));
   TypeData type_data(type);

   START_JSON
      JSON_STRING("kind", "ParmDecl")
      JSON_STRING("name", param_name)
      JSON_TYPE_DATA("type", type_data)
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visitFieldDecl(CXCursor cursor, CXCursor parent, Context* context) {
   string field_name = to_s_and_dispose(clang_getCursorSpelling(cursor));
   TypeData type_data(clang_getCursorType(cursor));
   CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(cursor);
   string usr = getCursorUSRString(cursor);

   START_JSON
      JSON_STRING("kind", "FieldDecl")
      JSON_STRING("name", field_name)
      JSON_TYPE_DATA("type", type_data)
      switch (access) {
      case CX_CXXPrivate:
         JSON_STRING("access", "private")
            break;
      case CX_CXXPublic:
         JSON_STRING("access", "public")
            break;
      case CX_CXXProtected:
         JSON_STRING("access", "protected")
            break;
      case CX_CXXInvalidAccessSpecifier:
        JSON_STRING("access", "invalid")
        break;
      }
      JSON_STRING("member_of", context->current_type())
      JSON_STRING("usr", usr)
   END_JSON

   return CXChildVisit_Recurse;
}

CXChildVisitResult visit(CXCursor cursor, CXCursor parent, CXClientData data) {
   if (!clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) {
      return CXChildVisit_Continue;
   }

   Context* context = (Context*)data;

   CXChildVisitResult result = CXChildVisit_Recurse;

   switch (cursor.kind) {
   case CXCursor_TypedefDecl:
      result = visitTypedefDecl(cursor, parent, context);
      break;
   case CXCursor_MacroDefinition:
      result = visitMacroDefinition(cursor, parent, context);
      break;
   case CXCursor_Namespace:
      result = visitNamespace(cursor, parent, context);
      break;
   case CXCursor_ClassTemplate:
      result = visitClassTemplate(cursor, parent, context);
      break;
   case CXCursor_UnionDecl:
      result = visitUnionDecl(cursor, parent, context);
      break;
   case CXCursor_EnumDecl:
      result = visitEnumDecl(cursor, parent, context);
      break;
   case CXCursor_EnumConstantDecl:
      result = visitEnumConstantDecl(cursor, parent, context);
      break;
   case CXCursor_ClassDecl:
      result = visitClassDecl(cursor, parent, context);
      break;
   case CXCursor_StructDecl:
      result = visitStructDecl(cursor, parent, context);
      break;
   // TODO: Base Specifier is never being hit for some reason.
   case CXCursor_CXXBaseSpecifier:
      result = visitCXXBaseSpecifier(cursor, parent, context);
      break;
   case CXCursor_FieldDecl:
      result = visitFieldDecl(cursor, parent, context);
      break;
   case CXCursor_CXXMethod:
      result = visitCXXMethod(cursor, parent, context);
      break;
   case CXCursor_FunctionDecl:
      result = visitFunctionDecl(cursor, parent, context);
      break;
   case CXCursor_ParmDecl:
      result = visitParmDecl(cursor, parent, context);
      break;
   default:
      ; /* Ignore cursors we don't care about */
   }

   return result;
}

int main(int argc, char** argv) {
   Context context;
   CXIndex index = clang_createIndex(0, 0);

   if (argc == 1 || (argc == 2 && 0 == strcmp(argv[1], "--help"))) {
     printf(
       "Usage\n"
       "  clang2json [CLANG_OPTIONS]... FILE\n"
       "\n"
       "Options\n"
       "  CLANG_OPTIONS\n"
       "    Same options you would pass to clang to compile this file.\n"
       "    Ex: `-x c++` will tell clang this is a c++ file, not c.\n"
       "    See `clang --help` or `man clang` for more information.\n"
       "\n"
       "  FILE\n"
       "    The file to convert.\n"
       "\n"
       "Example\n"
       "  # Analyze multiple files, appending the declarations into a single file.\n"
       "  # This can (and probably should) be scripted if a large number of files are needed.\n"
       "  clang2json -x c++ -I apr-1.5.1 apr-1.5.1/apr.h > declations.json\n"
       "  clang2json -x c++ -I apr-1.5.1 apr-1.5.1/apr_file_io.h >> declarations.json\n"
     );
     exit(0);
   }

   context.translation_unit = clang_parseTranslationUnit(index, 0, argv, argc, 0, 0, CXTranslationUnit_DetailedPreprocessingRecord);

   if (NULL == context.translation_unit) {
      cout << "Error parsing translation unit." << endl;
   }

   CXCursor cursor = clang_getTranslationUnitCursor(context.translation_unit);

   clang_visitChildren(cursor, visit, &context);

   clang_disposeTranslationUnit(context.translation_unit);
   clang_disposeIndex(index);
}
