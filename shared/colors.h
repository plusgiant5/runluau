#include <string>

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define RESET "\033[0m"

#define NIL_COLOR std::string(MAGENTA)
#define BOOLEAN_COLOR std::string(YELLOW)
#define NUMBER_COLOR std::string(GREEN)
#define STRING_COLOR std::string(CYAN)
#define FUNCTION_COLOR std::string(RED)
#define TABLE_COLOR std::string(BLACK)