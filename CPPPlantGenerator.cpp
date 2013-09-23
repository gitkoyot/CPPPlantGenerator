#include <iostream>
#include <string>
#include <sstream>
#include <clang-c/Index.h>
#include <vector>
#include <set>
#include <functional>
#include <map>
#include <cstdlib>
#include <cstring>

#include <boost/regex.hpp>


using namespace std;


//the following are UBUNTU/LINUX ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


/*

@startuml
class ThisIsAssociatedClass{
}
namespace LinkDelay #DDDDDD
class ConfigurationInternalData{
input_interface   :   std::string
output_interface   :   std::string
mac_address_string   :   std::string
input_filter_string   :   std::string
mac_address   :   char [6]
use_custom_mac   :   bool
link_delay   :   int
verbose_flag   :   int
allowed_to_finish   :   bool
exceptions   :   std::vector<std::shared_ptr<boost::exception_ptr> >
}
class Configuration{
void parseArguments ( argc   :   int , argv   :   char *[])
bool convert_string_to_mac ( mac_address_string   :   std::string , mac_address   :   char [])
std::string getInterfaceList ( )
data_static_composition   :   LinkDelay::ConfigurationInternalData
data   :   LinkDelay::ConfigurationInternalData *
me   :   LinkDelay::Configuration *
void create ( argc   :   int , argv   :   char *[])
LinkDelay::Configuration * instance ( )
boost::exception_ptr * getMyOwnException ( )
void rethrowExceptions ( )
void function ( dummy   :   ThisIsAssociatedClass *)
const std::string & getOutputInterface ( )
const std::string & getInputInterface ( )
const std::string & getInputFilterString ( )
bool may_i_die ( )
const std::string getInterface ( )
const std::string getFilterString ( )
void fatal_error ( )
void setAllowedToFinish ( allowedToFinish   :   bool)
bool getUseCustomMac ( )
int getLinkDelay ( )
const char * getMacAddress ( )
}
end namespace

LinkDelay.Configuration o-- LinkDelay.ConfigurationInternalData

LinkDelay.ConfigurationInternalData *-- std.string

LinkDelay.Configuration --> ThisIsAssociatedClass

@enduml





 */


////////////////////////////////////////////////////////////////////////////////
static stringstream output;
static int numspaces = 0;
bool descInnerFile = false;
bool descDependencies = false;
bool genDiagnostics = false;
typedef vector<CXCursor> dependenciesList;
dependenciesList dependecies;
boost::regex file_regex; //because std still sucks
const string extensions("(\\.cpp|\\.h|\\.c)");
////////////////////////////////////////////////////////////////////////////////
std::string
getCursorText(CXCursor cur)
{
  CXSourceRange range = clang_getCursorExtent(cur);
  CXSourceLocation begin = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);
  CXFile cxFile;
  unsigned int beginOff;
  unsigned int endOff;
  clang_getExpansionLocation(begin, &cxFile, 0, 0, &beginOff);
  clang_getExpansionLocation(end, 0, 0, 0, &endOff);
  CXString cx_filename = clang_getFileName(cxFile);
  const char* filename = clang_getCString(cx_filename);
  unsigned int textSize = endOff - beginOff;

  FILE * file = fopen(filename, "r");
  if (file == 0)
    {
      exit(-1);
    }
  fseek(file, beginOff, SEEK_SET);
  char buff[4096];
  char * pBuff = buff;
  if (textSize + 1 > sizeof(buff))
    {
      pBuff = new char[textSize + 1];
    }
  pBuff[textSize] = '\0';
  fread(pBuff, 1, textSize, file);
  std::string res(pBuff);
  if (pBuff != buff)
    {
      delete[] pBuff;
    }
  fclose(file);
  return res;
}
////////////////////////////////////////////////////////////////////////////////
static int
displayCursorInfo(CXCursor Cursor)
{
  if (Cursor.kind == CXCursor_MacroDefinition)
    {
      printf(" ==>\n");
      if (Cursor.kind == CXCursor_MacroDefinition)
        printf("CXCursor_MacroDefinition\n");
      if (Cursor.kind == CXCursor_MacroExpansion)
        printf("CXCursor_MacroExpansion\n");
      if (Cursor.kind == CXCursor_MacroInstantiation)
        printf("CXCursor_MacroInstantiation\n");

      CXString String = clang_getCursorDisplayName(Cursor);
      printf("Display: [%s]\n", clang_getCString(String));
      clang_disposeString(String);

      clang_getCursorExtent(Cursor);
      CXSourceLocation loc = clang_getCursorLocation(Cursor);

      CXFile file;
      unsigned line, col, off;
      clang_getSpellingLocation(loc, &file, &line, &col, &off);

      CXString strFileName = clang_getFileName(file);
      printf("Location: %s, %u:%u:%u\n", clang_getCString(strFileName), line,
          col, off);
      clang_disposeString(strFileName);
    }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
void
parse_field_or_argument(const CXCursor& cursor)
{
  CXString cursor_kind_spelling = clang_getCursorKindSpelling(cursor.kind);
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  const char *cursor_spelling_string = clang_getCString(cursor_spelling);

  CXType cursor_type = clang_getCursorType(cursor);
  CXString cursor_type_spelling = clang_getTypeSpelling(cursor_type);
  const char *cursor_type_spelling_string = clang_getCString(
      cursor_type_spelling);

  output << cursor_spelling_string << "   :   " << cursor_type_spelling_string;

  clang_disposeString(cursor_spelling);
  clang_disposeString(cursor_type_spelling);
}
////////////////////////////////////////////////////////////////////////////////
bool
emit_on_cursor_enter(const CXCursor& cursor)
{
  bool ret_value = true;
  CXString cursor_kind_spelling = clang_getCursorKindSpelling(cursor.kind);
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  const char *cursor_spelling_string = clang_getCString(cursor_spelling);

  CXType cursor_type = clang_getCursorType(cursor);
  CXString cursor_type_spelling = clang_getTypeSpelling(cursor_type);
  const char *cursor_type_spelling_string = clang_getCString(
      cursor_type_spelling);

  if (CXCursor_Namespace == cursor.kind)
    {
      output << "namespace " << cursor_spelling_string << " #DDDDDD" << endl;
    }

  if (CXCursor_ClassDecl == cursor.kind || CXCursor_StructDecl == cursor.kind)
    {
      output << "class " << cursor_spelling_string << "{" << endl;
    }

  if (CXCursor_FieldDecl == cursor.kind || CXCursor_VarDecl==cursor.kind)
    {
      CXCursor lexical_cursor_parent = clang_getCursorLexicalParent(cursor);

      // variable declaration in translation unit is not of our interest
      if (CXCursor_TranslationUnit != lexical_cursor_parent.kind)
        {

          parse_field_or_argument(cursor);
          dependecies.push_back(cursor);
          output << endl;
          // dont care about field guts - go to next sibling
          ret_value = false;
        }
    }

  if (CXCursor_CXXMethod == cursor.kind)
    {
      int num_arguments = clang_Cursor_getNumArguments(cursor);
      CXType result_type = clang_getResultType(cursor_type);
      CXString result_type_spelling = clang_getTypeSpelling(result_type);
      const char * result_type_spelling_string = clang_getCString(
          result_type_spelling);

      output << result_type_spelling_string << " " << cursor_spelling_string
          << " ( ";

      for (int i = 0; i < num_arguments; i++)
        {
          const CXCursor argument = clang_Cursor_getArgument(cursor, i);
          dependecies.push_back(argument);
          parse_field_or_argument(argument);

          if (num_arguments - 1 != i)
            {
              output << " , ";
            }
        }
      output << ")" << endl;

      clang_disposeString(result_type_spelling);
      ret_value = false;
    }

  clang_disposeString(cursor_kind_spelling);
  clang_disposeString(cursor_spelling);
  clang_disposeString(cursor_type_spelling);

  return ret_value;
}
////////////////////////////////////////////////////////////////////////////////
void
emit_on_cursor_exit(const CXCursor& cursor)
{
  if (CXCursor_Namespace == cursor.kind)
    {
      output << "end namespace " << endl;
    }

  if (CXCursor_ClassDecl == cursor.kind || CXCursor_StructDecl == cursor.kind)
    {
      output << "}" << endl;
    }

}
////////////////////////////////////////////////////////////////////////////////
void describeCursor (const CXCursor& cursor,ostream& out)
{
  CXFile file;
  unsigned int line;
  unsigned int column;
  unsigned int offset;
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  clang_getFileLocation(loc, &file, &line, &column, &offset);

  CXString cursor_file_location = clang_getFileName(file);
  const char* cursor_file_location_string = clang_getCString(
      cursor_file_location);

  std::string filename =
      cursor_file_location_string != nullptr ? cursor_file_location_string : "";

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
  CXSourceRange range = clang_getCursorExtent(cursor);

  CXString cursor_kind_spelling = clang_getCursorKindSpelling(cursor.kind);
  CXString cursor_spelling = clang_getCursorSpelling(cursor);

  CXType cursor_type = clang_getCursorType(cursor);
  CXString cursor_type_spelling = clang_getTypeSpelling(cursor_type);


  out << RED << clang_getCString(cursor_kind_spelling) << RESET <<
      GREEN << " F: " << filename << RESET
      << " L: " << line << " C: " << column << " O: " << offset << " RB "
      << range.begin_int_data << " RE " << range.end_int_data << " NARG "
      << clang_Cursor_getNumArguments(cursor) << " T: "
      << clang_getCString(cursor_type_spelling) << " C: " << YELLOW
      << clang_getCString(cursor_spelling) << RESET<< std::endl;


  clang_disposeString(cursor_kind_spelling);
  clang_disposeString(cursor_spelling);
  clang_disposeString(cursor_type_spelling);


}

////////////////////////////////////////////////////////////////////////////////
CXChildVisitResult
node_visitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
  std::string &fileName(*static_cast<std::string*>(clientData));

  CXFile file;
  unsigned int line;
  unsigned int column;
  unsigned int offset;
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  clang_getFileLocation(loc, &file, &line, &column, &offset);

  CXString cursor_file_location = clang_getFileName(file);
  const char* cursor_file_location_string = clang_getCString(
      cursor_file_location);

  std::string filename =
      cursor_file_location_string != nullptr ? cursor_file_location_string : "";

  clang_disposeString(cursor_file_location);

  if (false == boost::regex_match(filename, file_regex))
    return CXChildVisit_Continue;

  if (descInnerFile)
    {
      std::cerr << string(numspaces, ' ');

      describeCursor(cursor, std::cerr);
    }

  bool do_we_want_to_go_deeper = emit_on_cursor_enter(cursor);

  if (true == do_we_want_to_go_deeper)
    {
      numspaces++;
      clang_visitChildren(cursor, node_visitor, (CXClientData) &fileName);
      numspaces--;
    }

  emit_on_cursor_exit(cursor);

  return CXChildVisit_Continue;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * This function should print cursor with all parents (namespaces, structures
 * etc..)
 * @param cursor
 * @return
 */
string print_w_upper_namespaces(const CXCursor& cursor)
{
  auto semantic_cursor_parent = clang_getCursorSemanticParent(cursor);
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  string retValue = clang_getCString(cursor_spelling);

  switch (semantic_cursor_parent.kind)
  {
  case CXCursor_StructDecl:
  case CXCursor_ClassDecl:
  case CXCursor_Namespace:
    retValue = print_w_upper_namespaces(semantic_cursor_parent)+"."+retValue;
    break;
  default:
    break;
  }
  return retValue;

}
////////////////////////////////////////////////////////////////////////////////
struct compareCXCursorsS
{
  bool
  operator()(const CXCursor& __x, const CXCursor& __y) const
  {
    bool retVal = clang_equalCursors(__x, __y) != 0;
    if (retVal)
      retVal=false; // if elements are equal than they are not less
    else
      // elements are not equal, establish some order
      retVal = memcmp(&__x,&__y, sizeof(__x))<0;

    return retVal;
  }
};

////////////////////////////////////////////////////////////////////////////////
typedef set<CXCursor, compareCXCursorsS> uniqueCursorList;
typedef map<CXCursor, uniqueCursorList, compareCXCursorsS> uniqueMapping;
////////////////////////////////////////////////////////////////////////////////
void
handle_dependency_list(const uniqueMapping& dependencies_list, string plantSign)
{
  for (auto key : dependencies_list)
    {
      const CXCursor& source_dependency = key.first;
      for (auto destination_dependency : key.second)
        {

          output << print_w_upper_namespaces(source_dependency) << plantSign
              << print_w_upper_namespaces(destination_dependency) << endl;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void handle_dependencies(dependenciesList& dep)
{

  /* auto compareCXCursors = [](const CXCursor __x, const CXCursor __y)->bool const
   {
   return clang_equalCursors(__x,__y)!=0?true:false;
   };
   */
  std::function<bool
  (const CXCursor __x, const CXCursor __y)> const F =
      [](const CXCursor __x, const CXCursor __y)->bool const
        {
          return clang_equalCursors(__x,__y)!=0?true:false;
        };

  //typedef set<const CXCursor*, decltype(compareCXCursors)> uniqueCursorList;
  //typedef map<const CXCursor*, uniqueCursorList, decltype(compareCXCursors)> uniqueMapping;

  uniqueMapping associations;
  uniqueMapping aggregaions;
  uniqueMapping compositions;

  uniqueMapping *where_to_add = &associations; // assume some default

  /**
   * This loop should add source_object <relation> destination_object associations
   * to mapping
   * */
  for (const CXCursor dependencies : dep)
    {
      CXCursor destination_cursor;
      bool are_we_interested = false;
      CXCursor semantic_cursor_parent = clang_getCursorSemanticParent(
          dependencies);
      CXCursor lexical_cursor_parent = clang_getCursorLexicalParent(
          dependencies);
      CXCursor source_cursor = semantic_cursor_parent;

      if (descDependencies)
        {
          std::cerr << "DEP: ";
          describeCursor(dependencies, std::cerr);
          std::cerr << "  SEMP: ";
          describeCursor(semantic_cursor_parent, std::cerr);
          std::cerr << " LEXP: ";
          describeCursor(lexical_cursor_parent, std::cerr);
        }

      if (CXCursor_TranslationUnit != lexical_cursor_parent.kind)
        {

          if (CXCursor_ClassDecl == semantic_cursor_parent.kind
              || CXCursor_StructDecl == semantic_cursor_parent.kind)
            {
              where_to_add = &compositions;
              CXType cursor_type = clang_getCursorType(dependencies);

              if (descDependencies)
                cerr << RED << " ==> cursor type kind: " << cursor_type.kind
                    << RESET << endl;

              if (CXType_Pointer == cursor_type.kind)
                {
                  where_to_add = &aggregaions;
                  cursor_type = clang_getPointeeType(cursor_type);
                  if (descDependencies)
                    cerr << GREEN << " =====> pointed type kind: "
                        << cursor_type.kind << RESET << endl;
                }

              CXCursor cursor_to_declaration = clang_getTypeDeclaration(
                  cursor_type);

              destination_cursor = cursor_to_declaration;
              if (descDependencies)
                {
                  cerr << " aggregation this is declared here: " << endl
                      << YELLOW << " --> ";
                  describeCursor(cursor_to_declaration, cerr);
                }
              are_we_interested = true;

              //aggregation / composition
            }

          if (CXCursor_CXXMethod == semantic_cursor_parent.kind)
            {
              where_to_add = &associations;
              // get parent of method (this should be class or something
              semantic_cursor_parent = clang_getCursorSemanticParent(
                  semantic_cursor_parent);
              source_cursor = semantic_cursor_parent;

              CXType cursor_type = clang_getCursorType(dependencies);
              // association
              if (descDependencies)
                cerr << RED << " ==> cursor type kind: " << cursor_type.kind
                    << RESET << endl;
              if (cursor_type.kind > 100)
                {
                  if (CXType_Pointer == cursor_type.kind)
                    {

                      cursor_type = clang_getPointeeType(cursor_type);
                      if (descDependencies)
                        cerr << GREEN << " =====> pointed type kind: "
                            << cursor_type.kind << RESET << endl;
                    }

                  CXCursor cursor_to_declaration = clang_getTypeDeclaration(
                      cursor_type);
                  if (descDependencies)
                    {
                      cerr << " association is declared here: " << endl
                          << YELLOW << " --> ";
                      describeCursor(cursor_to_declaration, cerr);
                    }

                  destination_cursor = cursor_to_declaration;
                  are_we_interested = true;
                }

            }
        }

      if (are_we_interested && CXCursor_NoDeclFound != destination_cursor.kind)
        {
          if (descDependencies)
            {
              cerr << "Adding to dependencies" << endl;
              cerr << "Source " ;describeCursor(source_cursor, cerr);cerr<<endl;
              cerr << "Destination " ;describeCursor(destination_cursor, cerr);cerr<<endl;
            }

          (*where_to_add)[source_cursor].insert(destination_cursor);
          if (descDependencies)
            cerr << "Size after add " << (*where_to_add)[source_cursor].size() << endl;

        }
    }
  if (aggregaions.size() > 0)
    {
      output << endl;
      handle_dependency_list(aggregaions, " o-- ");
    }
  if (compositions.size() > 0)
    {
      output << endl;
      handle_dependency_list(compositions, " *-- ");
    }
  if (associations.size() > 0)
    {
      output << endl;
      handle_dependency_list(associations, " --> ");
    }

}
////////////////////////////////////////////////////////////////////////////////
void boost::throw_exception(std::exception const & e) {

}

int
main(int argc, char *argv[])
{
  // extract first parameter as filename
  string fileName(argv[1]);
  for (int i=1; i<argc; i++)
    argv[i]=argv[i+1];
  argc--;

  genDiagnostics = getenv("CPPPLANTDIAG")!=0;
  descInnerFile = getenv("CPPLANTINNERFILE")!=0;
  descDependencies = getenv("CPPLANTDEPENDENCIES")!=0;

  // construct regex for matching filenames without extension
  string r = ".*"+boost::regex_replace(fileName,boost::regex(extensions,boost::regex_constants::extended | boost::regex_constants::icase),extensions);
  file_regex = boost::regex(r);


  CXIndex Index = clang_createIndex(0, 0);

  CXTranslationUnit TU = clang_parseTranslationUnit(Index, fileName.c_str(),
      argv, argc, 0, 0, CXTranslationUnit_None);
  /**
   * Spit out diagnostics
   */
  if (genDiagnostics)
    for (unsigned I = 0, N = clang_getNumDiagnostics(TU); I != N; ++I)
      {
        CXDiagnostic Diag = clang_getDiagnostic(TU, I);
        CXString String = clang_formatDiagnostic(Diag,
            clang_defaultDiagnosticDisplayOptions());
        fprintf(stderr, "%s\n", clang_getCString(String));
        clang_disposeString(String);
      }

  output << "@startuml" << endl;

  clang_visitChildren(clang_getTranslationUnitCursor(TU), node_visitor,
      reinterpret_cast<CXClientData>(&fileName));

  handle_dependencies(dependecies);

  output << "@enduml" << endl;

  cout << output.str() << endl;

  clang_disposeTranslationUnit(TU);
  clang_disposeIndex(Index);
  return 0;
}
