/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2013 Eivind Kvedalen LGPL           */

/* Represents the many different ways we can access our data */
%union {
Expression * expr;
Path * label;
Path::Component * path;
int value;
char * stem;
}
%{

#if 0
union yystype {
Expression * expr;
Path * label;
Path::Component * path;
int value;
char * stem;
}
#endif

std::deque<Path::Component> components;

       #define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror
%}

     /* Bison declarations.  */
     %token ACOS ASIN ATAN ATAN2 COS COSH EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT
     %token NUM
     %token LABEL
     %token UNIT
     %token INTEGER
     %token CELLADDRESS
     %type <path> path
     %type <label> label
     %type <expr> input exp unit_exp func
     %type <stem> CELLADDRESS
     %type <stem> LABEL
     %type <expr> NUM ACOS ASIN ATAN ATAN2 COS COSH EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT UNIT
     %type <value> INTEGER
     %left '-' '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */
     %left NUM
     %left INTEGER

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true;                                        }
     |     unit_exp                             { ScanResult = $1; unitExpression = true;                                         }
     ;

exp:      NUM                			{ $$ = $1;                                                                        }
        | INTEGER                               { $$ = new NumberExpression(DocumentObject, (double)$1);                          }
        | NUM unit_exp                          { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::UNIT, $2);   }
        | INTEGER unit_exp                      { $$ = new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, (double)$1), OperatorExpression::UNIT, $2);   }
        | { components.clear(); } label                                 { $$ = new VariableExpression(DocumentObject, *$2); delete $2;                     }
        | exp '+' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::ADD, $3);   }
        | exp '-' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::SUB, $3);   }
        | exp '*' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | exp '/' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '/' unit_exp                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | '-' exp  %prec NEG 			{ $$ = new OperatorExpression(DocumentObject,
                                                                              new NumberExpression(DocumentObject, -1.0),
                                                                              OperatorExpression::MUL, $2);        	          }
        | exp '^' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | '(' exp ')'     			{ $$ = $2;                                                                        }
        | func                 			{ $$ = $1;                                                                        }
        ;

func:     ACOS  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ACOS, $3);   	  }
        | ASIN  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ASIN, $3);   	  }
        | ATAN  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ATAN, $3);   	  }
        | ATAN2 '(' exp ',' exp ')'             { $$ = new FunctionExpression(DocumentObject, FunctionExpression::ATAN2, $3, $5); }
        | ABS  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ABS, $3);   	  }
        | EXP  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::EXP, $3);       }
        | MOD '(' exp ',' exp ')'               { $$ = new FunctionExpression(DocumentObject, FunctionExpression::MOD, $3, $5);   }
        | LOG  '(' exp ')'			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::LOG, $3);   	  }
        | LOG10  '(' exp ')'			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::LOG10, $3);     }
        | SIN  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SIN, $3);       }
        | SINH '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SINH, $3);   	  }
        | POW '(' exp ',' exp ')'               { $$ = new FunctionExpression(DocumentObject, FunctionExpression::POW, $3, $5);   }
        | TAN  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::TAN, $3);   	  }
        | TANH  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::TANH, $3);   	  }
        | SQRT  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SQRT, $3);      }
        | COS  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::COS, $3);   	  }
        | COSH  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::COSH, $3);   	  }
        ;

unit_exp: UNIT                                  { $$ = new UnitExpression(DocumentObject, unit, unitstr, scaler);                 }
        | unit_exp '/' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' NUM                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | unit_exp '^' '-' NUM %prec NEG        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $4);
                                                  static_cast<NumberExpression*>($4)->negate();
                                                }
        | unit_exp '^' INTEGER                  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, (double)$3));   }
        | unit_exp '^' '-' INTEGER %prec NEG    { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, -(double)$4));  }
        | '(' unit_exp ')'                      { $$ = $2;                                                                        }
        ;

label: path                                     { $$ = new Path(); $$->addComponents(components);                                 }
     | LABEL ':' path                           { $$ = new Path(); $$->setDocumentObjectName($1); free($1);
                                                  $$->addComponents(components);                                                  }

path: LABEL                                     { components.push_front(Path::Component::SimpleComponent($1)); free($1);           }
     | CELLADDRESS                              { components.push_front(Path::Component::SimpleComponent($1)); free($1);           }
     | LABEL '{' LABEL '}'                      { components.push_front(Path::Component::MapComponent($1, $3)); free($1);          }
     | LABEL '[' INTEGER ']'                    { components.push_front(Path::Component::ArrayComponent($1, $3)); free($1);        }
     | LABEL '{' LABEL '}' '.' path             { components.push_front(Path::Component::MapComponent($1, $3)); free($1); free($3);}
     | LABEL '[' INTEGER ']' '.' path           { components.push_front(Path::Component::ArrayComponent($1, $3)); free($1);        }
     | LABEL '.' path                           { components.push_front(Path::Component::SimpleComponent($1)); free($1);           }
     ;

%%
