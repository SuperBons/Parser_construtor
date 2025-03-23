#include "parser.h"

#include "first_plus.h"
#include "scanner.h"
#include "token.h"

#include <iostream>
#include <string>



//----------------------------------------------------------------------------------------------
// The Parser object.  Aside from the usual constructor / destructor pair, there is parse(),
// get_next_word(), 3 getters for the counts, and the rules for the grammer, starting with
// program_start().  For now, the first_plus_sets lives outside this class.  I do this for
// clarity.  With all of the subroutine calls of the actual grammar, it started to clutter
// the fail.  Later, after grading and some usage, I plan to refactor into two private impls,
// one per the first plus table and the other for the recursive routines.
//
// Failure is maintained in a global state.  Rather than break out of any recursive calls, I
// set the fail state and let the stack unwind naturally.  This anticipates the need for
// error correction / useful parse diagnostics.
//
// Finally, variable "Token current_word" holds the top of the Token stack.  All routines look
// to it when making decisions based on tokens.
//----------------------------------------------------------------------------------------------


Parser::Parser() :
  fail_state{false}, variable_count{0}, function_count{0}, statement_count{0},
  scanner{nullptr},
  current_word{ TokenType::INITIAL, std::string{""}, 0 }
{

  //----------------------------------------------------------------------------------------------
  // The first plus sets are statically allocated.  I could have placed them into
  // a giant initializer list and moved the initializtion to a compiler step, but
  // it becomes unreadable without variable assignment syntax on a deep nested
  // structure.  When the parser is fully vetted, I'll refactor so that this step
  // is not needed.
  //----------------------------------------------------------------------------------------------
  
  if( !first_plus_sets_initialized ) {

    initialize_first_plus_sets();
    first_plus_sets_initialized = true;

  }
}

Parser::~Parser() {
}

//----------------------------------------------------------------------------------------------
// Use the scanner and internally assign it within the Parse framework.  Now, the parser
// controls the main scan loop.  Descend into 'program_start'.  Return status of the parse.
//----------------------------------------------------------------------------------------------

bool Parser::parse( Scanner & scanner ) {

  this->scanner = &scanner;

  
#ifdef DEBUG

  this->scanner->debug_display_token( current_word );

#endif
  
  return( program_start() );

}

//----------------------------------------------------------------------------------------------
// This is a convenience routine which is pretty straightforward.  Any time a new token is
// needed this routine will fetch it.  It must skip over all meta statements and it must check
// to see if the stack contains more items.  If no more tokens are on the stack it must signal
// 'false' in the return call, which will lead to a parse error.
//----------------------------------------------------------------------------------------------

bool Parser::get_next_word( void ) {

  // Skip over meta-statements.
  
  while( scanner->has_more_tokens() ) {
    
#ifdef DEBUG

  this->scanner->debug_display_token( current_word );

#endif
  
    current_word = scanner->get_next_token();
    
    if( current_word.get_token_type() != TokenType::META_STATEMENT ) {
      
      return(true);

    }

  }

#ifdef DEBUG

  std::cout << "ran out of tokens" << std::endl;
  
#endif
  
  return( false );

}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
// Parse entrance: (sketch of the structure of these routines follows --
//
// Example grammar with the 3 types of productions seen by this program:
//
//   A  --> B c D         FIRST_PLUS( m )
//        | a             FIRST_PLUS( a )
//        | EPSILON       FIRST_PLUS( EPSILON r )
//
// bool A( void ) {
//
//   if( check_first_plus_set( current_word, FirstPlus::A_p0 ) ) { // Examine table to see to go down RHS.
//   
//                     //------------------------------------------------------------
//     if( B() ) {     // Match to the first production, call non-terminal B().
//                     //------------------------------------------------------------
// 
//       //------------------------------------------------------------
//       // See if the current token on the stack matches csymbol.
//       //------------------------------------------------------------
//       
//       if( (current_word.get_token_type() == SYMBOL) && (current_word.get_token_name() == 'csymbol') ) {
//       
//          //------------------------------------------------------------
//          // Token is used, grab the next.
// 	    //------------------------------------------------------------
// 	 
//          if( get_next_word() ) {
// 	 
//             //------------------------------------------------------------
//             // Descend to non-terminal D
// 	       //------------------------------------------------------------
// 	    
//             if( D() ) {
// 	    
//               //------------------------------------------------------------
//               // We've matched the first production at this point.
// 	      //------------------------------------------------------------
// 	      
//               return(true);
// 
//             }
// 
//          }
// 
//       }
// 
//       //------------------------------------------------------------
//       // The second production just needs to check for terminal 'a'.
//       //------------------------------------------------------------
//       
//    } else if( check_first_plus_set( current_word, FirstPlus::A_p1 ) ) {  // Examine table to see to go down RHS.
// 
//      if((current_word.get_token_type() == SYMBOL) && (current_word.get_token_name() == 'asymbol')) {
// 
//       //------------------------------------------------------------
//       // Fetch the next word.
//       //------------------------------------------------------------
//       
//       if( get_next_word() ) {
// 
//         //------------------------------------------------------------
//         // The second production matches.
//         //------------------------------------------------------------
// 
//         return(true);
// 
//       }
// 
//    } else if( check_first_plus_set( current_word, FirstPlus::A_p2 ) ) {  // Examine table to see to go down RHS.
// 
//       if((current_word.get_token_type() == SYMBOL) && (current_word.get_token_name() == 'rsymbol')) {
// 
//         //------------------------------------------------------------
//         // The EPSILON case must check to see if the token on the stack is valid.
// 	   // It must not consume the token by fetching another word. The third
// 	   // production matches.
//         //------------------------------------------------------------
// 
//         return(true);
// 
//    }
// 
//         //------------------------------------------------------------
//         // Could not match any of the 3 productions so A() is not matched and returns false.
//         //------------------------------------------------------------
// 
//    fail_state = true;   // Global fail state (within Parse object).
// 
//    return(false);
//    
// }
// 
//
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

bool Parser::program_start( void ) {

  //  <program_start>             --> <program> eof
  //                                | eof

#ifdef DEBUG

  std::cout << "<program_start>" << std::endl;
  
#endif
  
  if( get_next_word() ) {

    if( check_first_plus_set( current_word, FirstPlus::program_start_p0 ) ) {
 
      if( program() ) {
	if( current_word.get_token_type() == TokenType::EOF_TOK ) {
	  return(!fail_state);
	}
      }

    } else if( check_first_plus_set( current_word,  FirstPlus::program_start_p1 ) ) {
    
      if( current_word.get_token_type() == TokenType::EOF_TOK ) {
	
	return(!fail_state);
	
      }

    }

  }

  fail_state = true;

  return(false);
  
}

bool Parser::program(void) {

  // <program>                    --> <type_name> ID <program_0>                     FIRST_PLUS = { binary decimal int void }

  if( check_first_plus_set( current_word, FirstPlus::program_p0 ) ) {

    if ( type_name() ) {

      if ( (current_word.get_token_type() == TokenType::IDENTIFIER)  ) {

        if( get_next_word() ) {

          if ( program_0() ) {

            return( true );

          }

        }

      }

    }

  }

  fail_state = true;

  return(false);

}

bool Parser::type_name(void) {
  // <type_name> --> int | void | binary | decimal


  // Add your code here
  if (current_word.get_token_type()== TokenType::RESERVED_WORD && (current_word.get_token_name() == "int" ||
       current_word.get_token_name() == "void" ||
       current_word.get_token_name() == "binary" ||
       current_word.get_token_name() == "decimal")) {
    get_next_word();
    return true;
  }
  else{ 
    fail_state = true; 
  return false;
  }; 

}

// TODO: Implement this function
bool Parser::program_0(void) {
  // <program_0> --> <id_0> <id_list_0> semicolon <program_1>
  //            | left_parenthesis <func_0> <func_path>

  // Add your code here


  if (id_0()) {
    if (id_list_0()) {
      if (current_word.get_token_type() ==TokenType::SYMBOL && current_word.get_token_name() == ";") { get_next_word(); 
        if (program_1())  return true;
      }
    }
  }


  if (current_word.get_token_type()== TokenType::SYMBOL && current_word.get_token_name() == "(") {
    get_next_word();
    ++function_count;
    if (func_0()) {
      if (func_path()) { 
        return true;
      }
    }
    fail_state = true;
    return false;
  }
  

  fail_state = true;
  return false;
}


// TODO: Implement this function
bool Parser::id_0(void) {
  // <id_0> --> left_bracket <expression> right_bracket | EPSILON

  // Add your code here 

  if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "[") {
    get_next_word();
    if (!expression()) {fail_state = true; return false;}
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "]") { get_next_word(); return true;}
    fail_state = true;
    return false;
  }
  return true;
}



// TODO: Implement this function
bool Parser::id_list_0(void) {

  // <id_list_0>                  --> comma <id> <id_list_0>                     FIRST_PLUS = { comma }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON semicolon }

  // Add your code here
  // this will loop through until there's no ',', taking care of the epsilon 
  while(current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ",") { 
    get_next_word();  
    ++variable_count; 
    if (!id()) {  fail_state = true; return false; }
  }
  return true;
  
}


bool Parser::program_1(void) {

  // <program_1>                  --> <type_name> ID <func_or_data>                     FIRST_PLUS = { binary decimal int void }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON eof }

  if( check_first_plus_set( current_word, FirstPlus::program_1_p0 ) ) {

    if ( type_name() ) {

      if ( (current_word.get_token_type() == TokenType::IDENTIFIER)  ) {

        if( get_next_word() ) {

          if ( func_or_data() ) {
            ++function_count;
            return( true );

          }

        }

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::program_1_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::func_0(void) {

  // <func_0>                     --> <parameter_list> right_parenthesis <func_1>                     FIRST_PLUS = { binary decimal int void }
  //                                | right_parenthesis <func_4>                     FIRST_PLUS = { right_parenthesis }

  // Add your code here

    if (current_word.get_token_type() == TokenType::RESERVED_WORD && (current_word.get_token_name() == "binary" ||
       current_word.get_token_name() == "decimal" ||
       current_word.get_token_name() == "int" ||
       current_word.get_token_name() == "void")) {
    if(!parameter_list()) { fail_state = true; return false;}
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
      get_next_word(); 
      return func_1();
    } else {
      fail_state = true;
      return false;
    }
  }
  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
    get_next_word();
    return func_4();
  }
  fail_state =true;
  return false;
}

bool Parser::func_path(void) {

  // <func_path>                  --> <func_list>                     FIRST_PLUS = { binary decimal int void }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON eof }

  if( check_first_plus_set( current_word, FirstPlus::func_path_p0 ) ) {

    if ( func_list() ) {

      return( true );

    }

  } else if( check_first_plus_set( current_word, FirstPlus::func_path_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::expression(void) {

  // <expression>                 --> <factor> <term_0> <expression_0>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  // Add your code here

   if (factor() && term_0() && expression_0()) return true;
  
  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::id(void) {

  // <id>                         --> ID <id_0>                     FIRST_PLUS = { ID }

  // Add your code here

    if (current_word.get_token_type()== TokenType::IDENTIFIER) {
    get_next_word(); 
    return id_0();
  }


  fail_state = true;
  return false;

}

bool Parser::func_or_data(void) {

  // <func_or_data>               --> <id_0> <id_list_0> semicolon <program_1>                     FIRST_PLUS = { comma left_bracket semicolon }
  //                                | left_parenthesis <func_0> <func_list_0>                     FIRST_PLUS = { left_parenthesis }

  if( check_first_plus_set( current_word, FirstPlus::func_or_data_p0 ) ) {

    if ( id_0() ) {

      ++variable_count;
      
      if ( id_list_0() ) {

        if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == ";")  ) {

          if( get_next_word() ) {

            if ( program_1() ) {

              return( true );

            }

          }

        }

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::func_or_data_p1 ) ) {

    if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == "(")  ) {
      if( get_next_word() ) {
        ++function_count;
        if ( func_0() ) {

          if ( func_list_0() ) {

            return( true );

          }

        }

      }

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::parameter_list(void) {

  // <parameter_list>             --> void <parameter_list_0>                     FIRST_PLUS = { void }
  //                                | int ID <non_empty_list_0>                     FIRST_PLUS = { int }
  //                                | decimal ID <non_empty_list_0>                     FIRST_PLUS = { decimal }
  //                                | binary ID <non_empty_list_0>                     FIRST_PLUS = { binary }

  // Add your code here

  // take care of void get in 
  if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "void") {
    get_next_word(); 
    return parameter_list_0();
  }
  
  // now take caer of int, decimal, or binary 
  if (current_word.get_token_type() == TokenType::RESERVED_WORD && (current_word.get_token_name() == "int" ||
      current_word.get_token_name() == "decimal" || current_word.get_token_name() == "binary")) {
    if (!type_name()) { fail_state = true; return false;}

    // once there's a int, decimal, or binary there must be a identifier that follows if not it won't work 
    if (current_word.get_token_type() != TokenType::IDENTIFIER) { fail_state = true; return false; }
    get_next_word();  
    return non_empty_list_0();
  }
  
  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::func_1(void) {

  // <func_1>                     --> semicolon                     FIRST_PLUS = { semicolon }
  //                                | left_brace <func_2>                     FIRST_PLUS = { left_brace }


  if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
    get_next_word();  
    //++function_count;
    return true;

    fail_state = false; 
  }


  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "{") {
    get_next_word();  
    return func_2();
  }

  fail_state = true;
  return false; 
}

// TODO: Implement this function
bool Parser::func_4(void) {

  // <func_4>                     --> semicolon                     FIRST_PLUS = { semicolon }
  //                                | left_brace <func_5>                     FIRST_PLUS = { left_brace }

  // Add your code here

  if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
    get_next_word(); 
    return true;
    fail_state = false; 
  }

  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "{") {
    get_next_word();  
    return func_5(); 
  }
  
  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::func_list(void) {

  // <func_list>                  --> <func> <func_list_0>                     FIRST_PLUS = { binary decimal int void }

 // Add your code here

 
    if (func()) {
      //++function_count;
      return func_list_0();
    }
  
  
  fail_state = true;
  return false;
}

// TODO: Implement this function
bool Parser::factor(void) {

  // <factor>                     --> ID <factor_0>                     FIRST_PLUS = { ID }
  //                                | NUMBER                     FIRST_PLUS = { NUMBER }
  //                                | minus_sign NUMBER                     FIRST_PLUS = { minus_sign }
  //                                | left_parenthesis <expression> right_parenthesis                     FIRST_PLUS = { left_parenthesis }

  // Add your code here

    if (current_word.get_token_type() == TokenType::IDENTIFIER) { get_next_word();  
    return factor_0();



  }
  else if (current_word.get_token_type() == TokenType::NUMBER) {
    get_next_word();  
    return true;
  }

  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "-") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::NUMBER) {
      get_next_word();  
      return true;
    }  

    fail_state = true;
    return false;
  }
  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
    get_next_word(); 
    if (expression()) {

      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {

        get_next_word();  
        return true;
      }
    }


    fail_state = true;
    return false;
  }


  fail_state = true;
  return false;



}


bool Parser::term_0(void) {

  // <term_0>                     --> <mulop> <factor> <term_0>                     FIRST_PLUS = { forward_slash star_sign }
  //                                | EPSILON                     FIRST_PLUS = { != < <= == > >= EPSILON comma double_and_sign double_or_sign minus_sign plus_sign right_bracket right_parenthesis semicolon }

  if( check_first_plus_set( current_word, FirstPlus::term_0_p0 ) ) {

    if ( mulop() ) {

      if ( factor() ) {

        if ( term_0() ) {

          return( true );

        }

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::term_0_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}


bool Parser::expression_0(void) {

  // <expression_0>               --> <addop> <term> <expression_0>                     FIRST_PLUS = { minus_sign plus_sign }
  //                                | EPSILON                     FIRST_PLUS = { != < <= == > >= EPSILON comma double_and_sign double_or_sign right_bracket right_parenthesis semicolon }

  if( check_first_plus_set( current_word, FirstPlus::expression_0_p0 ) ) {

    if ( addop() ) {

      if ( term() ) {

        if ( expression_0() ) {

          return( true );

        }

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::expression_0_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}


bool Parser::func_list_0(void) {

  // <func_list_0>                --> <func_list>                     FIRST_PLUS = { binary decimal int void }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON eof }

  if( check_first_plus_set( current_word, FirstPlus::func_list_0_p0 ) ) {

    if ( func_list() ) {
      //++function_count; 

      return( true );

    }

  } else if( check_first_plus_set( current_word, FirstPlus::func_list_0_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::parameter_list_0(void) {

  // <parameter_list_0>           --> ID <non_empty_list_0>                     FIRST_PLUS = { ID }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON right_parenthesis }

  // Add your code here

    if (current_word.get_token_type() == TokenType::IDENTIFIER) {
    get_next_word(); 
    return non_empty_list_0();
  }; 
 
  return true;
}

// TODO: Implement this function
bool Parser::non_empty_list_0(void) {

  // <non_empty_list_0>           --> comma <type_name> ID <non_empty_list_0>                     FIRST_PLUS = { comma }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON right_parenthesis }

  // Add your code here

    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ",") {
    get_next_word();  
    if ( !type_name() ) {fail_state = true; return false; }; 
    if (current_word.get_token_type() != TokenType::IDENTIFIER) { fail_state = true; return false; }; 
    get_next_word(); 
    return non_empty_list_0();
  }; 
  return true;
}

// TODO: Implement this function
bool Parser::func_2(void) {

  // <func_2>                     --> <data_decls> <func_3>                     FIRST_PLUS = { binary decimal int void }
  //                                | <statements> right_brace                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | right_brace                     FIRST_PLUS = { right_brace }

  // Add your code here

  if (check_first_plus_set(current_word,FirstPlus::data_decls_p0)) {if (data_decls()) return func_3(); } 
  
  else if (check_first_plus_set(current_word, FirstPlus::statements_p0)) {
    if (statements()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
        get_next_word();  
        return true;
      }; 
    }; 
   }
    else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
    get_next_word(); 
    return true;
  }



  fail_state = true;
  return false;  
}

// TODO: Implement this function
bool Parser::func_5(void) {

  // <func_5>                     --> <data_decls> <func_6>                     FIRST_PLUS = { binary decimal int void }
  //                                | <statements> right_brace                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | right_brace                     FIRST_PLUS = { right_brace }

  // Add your code here


  // same thing as the last just with 

  if (check_first_plus_set(current_word, FirstPlus::data_decls_p0)) { if (data_decls()) return func_6();
  } 
  else if (check_first_plus_set(current_word, FirstPlus::statements_p0)) {
    if (statements()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
        get_next_word();  
        return true;
      }; 
    }; 
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
    get_next_word();  
    return true;
  }

  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::func(void) {

  // <func>                       --> <type_name> ID left_parenthesis <func_0>                     FIRST_PLUS = { binary decimal int void }

  // Add your code here
  if (type_name()) {
    if (current_word.get_token_type() == TokenType::IDENTIFIER) {
      get_next_word();  
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
        get_next_word();
        ++function_count; 
        return func_0();
      }; 
    }; 
  }

  fail_state = true;
  return false;

}


bool Parser::factor_1(void) {
  // <factor_1> --> <expr_list> right_parenthesis                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }
  //               | right_parenthesis                     FIRST_PLUS = { right_parenthesis }

  if (check_first_plus_set(current_word, FirstPlus::factor_1_p0)) {
    // Try first production: <expr_list> right_parenthesis
    if (expr_list()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
        get_next_word();
        return true;
      }
      fail_state = true;
      return false;
    }
    fail_state = true;
    return false;
  } 
  else if (check_first_plus_set(current_word, FirstPlus::factor_1_p1)) {
    // Try second production: right_parenthesis
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
      get_next_word();
      return true;
    }
    fail_state = true;
    return false;
  }

  fail_state = true;
  return false;
}


// TODO: Implement this function
bool Parser::factor_0(void) {

  // <factor_0>                   --> left_bracket <expression> right_bracket                     FIRST_PLUS = { left_bracket }
  //                                | left_parenthesis <factor_1>                     FIRST_PLUS = { left_parenthesis }
  //                                | EPSILON                     FIRST_PLUS = { != < <= == > >= EPSILON comma double_and_sign double_or_sign forward_slash minus_sign plus_sign right_bracket right_parenthesis semicolon star_sign }

  // Add your code here

    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "[") {
    get_next_word();  
    if (!expression()) {
    fail_state = true;
      return false;
    }
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "]") {
      get_next_word(); 
      return true;
    }
    fail_state = true;
    return false;
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
    get_next_word();  
    return factor_1();
  }


   fail_state = false; 
   return true;
  

}

// TODO: Implement this function
bool Parser::mulop(void) {

  // <mulop>                      --> star_sign                     FIRST_PLUS = { star_sign }
  //                                | forward_slash                     FIRST_PLUS = { forward_slash }

  // Add your code here

   if (current_word.get_token_type()  == TokenType::SYMBOL) {
    if (current_word.get_token_name() == "*" || current_word.get_token_name() == "/") {
      get_next_word();  
      fail_state = false;
      return true;
       
    }
  }

  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::addop(void) {

  // <addop>                      --> plus_sign                     FIRST_PLUS = { plus_sign }
  //                                | minus_sign                     FIRST_PLUS = { minus_sign }

  // Add your code here

    if (current_word.get_token_type() == TokenType::SYMBOL) {
      if (current_word.get_token_name()== "+" || current_word.get_token_name()== "-") {
      get_next_word(); 
      fail_state = false;
      return true;
       
    }
  }

  fail_state = true;
  return false;
}

// TODO: Implement this function
bool Parser::term(void) {

  // <term>                       --> <factor> <term_0>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  // Add your code here

  if(factor()) return term_0(); 

  fail_state = true; 
  return false; 
}

bool Parser::data_decls(void) {

  // <data_decls>                 --> <type_name> <id_list> semicolon <data_decls_0>                     FIRST_PLUS = { binary decimal int void }

  if( check_first_plus_set( current_word, FirstPlus::data_decls_p0 ) ) {

    if ( type_name() ) {

      if ( id_list() ) {

        if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == ";")  ) {

          if( get_next_word() ) {

            if ( data_decls_0() ) {

              return( true );

            }

          }

        }

      }

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::func_3(void) {

  // <func_3>                     --> <statements> right_brace                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | right_brace                     FIRST_PLUS = { right_brace }

  // Add your code here

    if (check_first_plus_set(current_word, FirstPlus::statements_p0)) {
    if (statements()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
        get_next_word(); 
        fail_state = false;
        return true;
      }
    }
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
    get_next_word(); 
    fail_state = false;
    return true;
  }

  fail_state = true;
  return false;



}

// TODO: Implement this function
bool Parser::statements(void) {

  // <statements>                 --> <statement> <statements_0>                     FIRST_PLUS = { ID break continue if print read return while write }

  // Add your code here

    if (statement()) return statements_0();
  


  fail_state = true;
  return false;

}

// TODO: Implement this function
bool Parser::func_6(void) {

  // <func_6>                     --> <statements> right_brace                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | right_brace                     FIRST_PLUS = { right_brace }

  // Add your code here
  
    if (check_first_plus_set(current_word, FirstPlus::statements_p0)) {
    if (statements()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
        get_next_word();  
        return true;
      }
    }
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
    get_next_word();  
    fail_state = false; 
    return true;
  }

  fail_state = true;
  return false;

}


bool Parser::id_list(void) {

  // <id_list>                    --> <id> <id_list_0>                     FIRST_PLUS = { ID }

  if( check_first_plus_set( current_word, FirstPlus::id_list_p0 ) ) {

    ++variable_count;
      
    if ( id() ) {

      if ( id_list_0() ) {
	
        return( true );

      }

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::data_decls_0(void) {

  // <data_decls_0>               --> <data_decls>                     FIRST_PLUS = { binary decimal int void }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON ID break continue if print read return right_brace while write }

  // Add your code here

    if (check_first_plus_set(current_word, FirstPlus::data_decls_0_p0)) return data_decls();
  

  fail_state = false; 
  return true;



}

// TODO: Implement this function
bool Parser::statement(void) {

  // <statement>                  --> ID <statement_0>                     FIRST_PLUS = { ID }
  //                                | if left_parenthesis <condition_expression> right_parenthesis <block_statements>                     FIRST_PLUS = { if }
  //                                | while left_parenthesis <condition_expression> right_parenthesis <block_statements>                     FIRST_PLUS = { while }
  //                                | return <statement_2>                     FIRST_PLUS = { return }
  //                                | break semicolon                     FIRST_PLUS = { break }
  //                                | continue semicolon                     FIRST_PLUS = { continue }
  //                                | read left_parenthesis ID right_parenthesis semicolon                     FIRST_PLUS = { read }
  //                                | write left_parenthesis <expression> right_parenthesis semicolon                     FIRST_PLUS = { write }
  //                                | print left_parenthesis STRING right_parenthesis semicolon                     FIRST_PLUS = { print }


  // Add your code here

  ++statement_count; 


   if (current_word.get_token_type() == TokenType::IDENTIFIER) { get_next_word(); return statement_0();
  } 
  
  else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() =="if") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
      get_next_word(); 
      if (condition_expression()) {
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
          get_next_word();  
          return block_statements();
        }
      }
    }
  }
   else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "while") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
      get_next_word();  
      if (condition_expression()) {
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
          get_next_word();  
          return block_statements();
        }
      }
    }
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "return") {
    get_next_word();  
    return statement_2();
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "break") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
      get_next_word();  
      fail_state = false; 
      return true;
      
    }
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "continue") {
    get_next_word(); 
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
      get_next_word();  
      return true;
    }
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "read") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
      get_next_word();  
      if (current_word.get_token_type() == TokenType::IDENTIFIER) {
        get_next_word(); 
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
          get_next_word(); 
          if(current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
            get_next_word();  
            return true;
          }
        }
      }
    }
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD &&  current_word.get_token_name() == "write") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
      get_next_word();  
      if (expression()) {
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
          get_next_word();  
          if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
            get_next_word();  
            return true;
          }
        }
      }
    }
  } else if (current_word.get_token_type() == TokenType::RESERVED_WORD && current_word.get_token_name() == "print") {
    get_next_word(); 
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
      get_next_word();  
      if (current_word.get_token_type() == TokenType::STRING) {
        get_next_word(); 
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
          get_next_word(); 
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
            get_next_word();  
            fail_state = false; 
            return true;
          
          }; 
        }; 
      }; 
    }; 
  }; 

  fail_state = true;
  return false;


  
}

// TODO: Implement this function
bool Parser::statements_0(void) {

  // <statements_0>               --> <statements>                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON right_brace }

  // Add your code here


    if (check_first_plus_set(current_word, FirstPlus::statements_0_p0)) return statements();
  

  fail_state = false; 
  return true;




}


bool Parser::expr_list(void) {

  // <expr_list>                  --> <non_empty_expr_list>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  if( check_first_plus_set( current_word, FirstPlus::expr_list_p0 ) ) {

    if ( non_empty_expr_list() ) {

      return( true );

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::statement_0(void) {

  // <statement_0>                --> equal_sign <expression> semicolon                     FIRST_PLUS = { equal_sign }
  //                                | left_bracket <expression> right_bracket equal_sign <expression> semicolon                     FIRST_PLUS = { left_bracket }
  //                                | left_parenthesis <statement_1>                     FIRST_PLUS = { left_parenthesis }

  // Add your code here


   if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "=") {
    get_next_word(); 
    if (expression()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
        get_next_word();  
        return true;
      }
    }
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "[") {
    get_next_word(); 
    if (expression()) {
      if (current_word.get_token_type() == TokenType::SYMBOL &&  current_word.get_token_name() == "]") {
        get_next_word();  
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "=") {
          get_next_word(); 
          if (expression()) {
            if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
              get_next_word(); 
              return true;
            }; 
          };
        };
      };
    };
  }
   else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "(") {
    get_next_word();    
    return statement_1();
  }

  fail_state = true;
  return false;


}


bool Parser::condition_expression(void) {

  // <condition_expression>       --> <condition> <condition_expression_0>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  if( check_first_plus_set( current_word, FirstPlus::condition_expression_p0 ) ) {

    if ( condition() ) {

      if ( condition_expression_0() ) {

        return( true );

      }

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::block_statements(void) {

  // <block_statements>           --> left_brace <block_statements_0>                     FIRST_PLUS = { left_brace }

  // Add your code here

   if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "{") {
    get_next_word();  
    return block_statements_0();
  }

  fail_state = true;
  return false;


}

// TODO: Implement this function
bool Parser::statement_2(void) {

  // <statement_2>                --> <expression> semicolon                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }
  //                                | semicolon                     FIRST_PLUS = { semicolon }

  // Add your code here

   if (check_first_plus_set(current_word, FirstPlus::statement_2_p0)) {
    if (expression()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
        get_next_word(); 
        fail_state = false; 
        return true;
      }
    }
    fail_state = true;
    return false;
  } 
  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
    get_next_word();
    fail_state = false;   
    return true;
  }

  fail_state = true;
  return false;

}


bool Parser::non_empty_expr_list(void) {

  // <non_empty_expr_list>        --> <expression> <non_empty_expr_list_0>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  if( check_first_plus_set( current_word, FirstPlus::non_empty_expr_list_p0 ) ) {

    if ( expression() ) {

      if ( non_empty_expr_list_0() ) {

        return( true );

      }

    }

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::statement_1(void) {

  // <statement_1>                --> <expr_list> right_parenthesis semicolon                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }
  //                                | right_parenthesis semicolon                     FIRST_PLUS = { right_parenthesis }

  // Add your code here

 if (check_first_plus_set(current_word, FirstPlus::expr_list_p0)) {
    if (expr_list()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
        get_next_word();  
        if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
          get_next_word();  
          return true;
        };
      };
    };
  }
  
  else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ")") {
    get_next_word();  
    if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == ";") {
      get_next_word();  
      fail_state = false; 
      return true;
    }
  }

  fail_state = true;
  return false;


}


bool Parser::condition(void) {

  // <condition>                  --> <expression> <comparison_op> <expression>                     FIRST_PLUS = { ID NUMBER left_parenthesis minus_sign }

  if( check_first_plus_set( current_word, FirstPlus::condition_p0 ) ) {

    if ( expression() ) {

      if ( comparison_op() ) {

        if ( expression() ) {

          return( true );

        }

      }

    }

  }

  fail_state = true;

  return(false);

}


bool Parser::condition_expression_0(void) {

  // <condition_expression_0>     --> <condition_op> <condition>                     FIRST_PLUS = { double_and_sign double_or_sign }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON right_parenthesis }

  if( check_first_plus_set( current_word, FirstPlus::condition_expression_0_p0 ) ) {

    if ( condition_op() ) {

      if ( condition() ) {

        return( true );

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::condition_expression_0_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::block_statements_0(void) {

  // <block_statements_0>         --> <statements> right_brace                     FIRST_PLUS = { ID break continue if print read return while write }
  //                                | right_brace                     FIRST_PLUS = { right_brace }


  // Add your code here



  if (check_first_plus_set(current_word, FirstPlus::statements_p0)) {
    if (statements()) {
      if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
        get_next_word();  
        fail_state = false; 
        return true;
      }
    }
  } else if (current_word.get_token_type() == TokenType::SYMBOL && current_word.get_token_name() == "}") {
    get_next_word();  
    fail_state = false; 
    return true;
  }

  fail_state = true;
  return false;




}


bool Parser::non_empty_expr_list_0(void) {

  // <non_empty_expr_list_0>      --> comma <expression> <non_empty_expr_list_0>                     FIRST_PLUS = { comma }
  //                                | EPSILON                     FIRST_PLUS = { EPSILON right_parenthesis }

  if( check_first_plus_set( current_word, FirstPlus::non_empty_expr_list_0_p0 ) ) {

    if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == ",")  ) {

      if( get_next_word() ) {

        if ( expression() ) {

          if ( non_empty_expr_list_0() ) {

            return( true );

          }

        }

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::non_empty_expr_list_0_p1 ) ) {

      // EPSILON consumes no tokens.

    return( true );

  }

  fail_state = true;

  return(false);

}

// TODO: Implement this function
bool Parser::comparison_op(void) {

  // <comparison_op>              --> ==                     FIRST_PLUS = { == }
  //                                | !=                     FIRST_PLUS = { != }
  //                                | >                     FIRST_PLUS = { > }
  //                                | >=                     FIRST_PLUS = { >= }
  //                                | <                     FIRST_PLUS = { < }
  //                                | <=                     FIRST_PLUS = { <= }

  // Add your code here
   if (current_word.get_token_type() == TokenType::SYMBOL) {
    if (current_word.get_token_name() == "==" || current_word.get_token_name() == "!=" ||
        current_word.get_token_name() == ">" || current_word.get_token_name() == ">=" ||
        current_word.get_token_name() == "<" || current_word.get_token_name() == "<=") {
      get_next_word();
      return true;
    }
  }
  
  fail_state = true;
  return false;



}

bool Parser::condition_op(void) {

  // <condition_op>               --> double_and_sign                     FIRST_PLUS = { double_and_sign }
  //                                | double_or_sign                     FIRST_PLUS = { double_or_sign }

  if( check_first_plus_set( current_word, FirstPlus::condition_op_p0 ) ) {

    if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == "&&")  ) {

      if( get_next_word() ) {

        return( true );

      }

    }

  } else if( check_first_plus_set( current_word, FirstPlus::condition_op_p1 ) ) {

    if ( (current_word.get_token_type() == TokenType::SYMBOL) && (current_word.get_token_name() == "||")  ) {

      if( get_next_word() ) {

        return( true );

      }

    }

  }

  fail_state = true;

  return(false);

}