grammar Lua55Grammar;

prog: block EOF ;

// chunk ::= block
// block ::= {stat} [retstat]
block: (statement ';'? )* (returnStatement ';'? )?;

statement:
// stat ::=  ‘;’ | 
    emptyStatement
//         do block end | 
    | doBlockStatement
//         varlist ‘=’ explist | 
    | assignmentStatement
//         local attnamelist [‘=’ explist] | 
//         global attnamelist | 
    | declarationStatement
//         global [attrib] ‘*’ 
    | globalAttribStatement
//         function funcname funcbody | 
//         local function Name funcbody | 
//         global function Name funcbody | 
    | funcdefStatement
//         while exp do block end | 
    | whileStatement
//         repeat block until exp | 
    | repeatStatement
//         if exp then block {elseif exp then block} [else block] end | 
    | ifStatement
//         for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end | 
    | numericForStatement
//         for namelist in exp do block end | 
    | genericForStatement
//         goto Name | 
    | gotoStatement
//         label | 
    | labelStatement
//         break | 
    | breakStatement
//         functioncall | 
    | funcCallStatement
;

emptyStatement: ';';

doBlockStatement: DO block END ;

assignmentStatement: varlist '=' explist ;
varlist: var (',' var)* ;
explist: exp (',' exp)* ;



// declarationStatement: scopeSpec attrib? attnamelist ('=' explist)? ;
declarationStatement: scopeSpec attnamelist ('=' explist)? ;
attnamelist: nameattr (',' nameattr)* ;

globalAttribStatement: GLOBAL ATTRIB ASTRIX_SIGN ; 

scopeSpec: GLOBAL | LOCAL;

funcdefStatement: 
    defaultFuncdefStatement
    | scopedFuncdefStatement
;
defaultFuncdefStatement: FUNCTION funcname funcbody; 
scopedFuncdefStatement: scopeSpec FUNCTION name funcbody; 

funcname: namespec (':' name)? ;
namespec: name ('.' name)* ;
funcbody: '(' paramlist? ')' block END ;

whileStatement: WHILE exp DO block END ;
repeatStatement: REPEAT block UNTIL exp ;
ifStatement: IF exp THEN block (ELSEIF exp THEN block)* (ELSE block)? END;
numericForStatement: FOR name '=' exp ',' exp (',' exp)? DO block END;
genericForStatement: FOR namelist IN explist DO block END ;
gotoStatement: GOTO name ;
labelStatement: '::' name '::';
breakStatement: BREAK;
returnStatement: RETURN explist?;

funcCallStatement: funcCall ;


funcAnon: FUNCTION funcbody;

tableConstructor: '{' fieldlist '}' | '{' '}';

exp: 
    literal
    | funcAnon
    | prefixexp
    | tableConstructor
    | opExp
;

opExp: orExp;
orExp: andExp (OR andExp)* ;
andExp: compExp (AND compExp)* ;
compExp: bitorExp (compop bitorExp)? ;
bitorExp: bitxorExp ('|' bitxorExp)* ;
bitxorExp: bitandExp (TILDA_SIGN bitandExp)* ;
bitandExp: shiftExp ('&' shiftExp)* ;
shiftExp: concatExp (shiftop concatExp)* ;
concatExp: plusExp ('..' plusExp)* ;
plusExp: mulExp (plusop mulExp)* ;
mulExp: unaryExp (mulop unaryExp)* ;
unaryExp: powExp | unop unaryExp;
powExp: opStartExp ('^' powExp)? ;

opStartExp: 
    prefixexp
    | literal
    | tableConstructor
;

literal: 
    NIL 
    | TRUE
    | FALSE
    | NUMBER
    | STRING
;

prefixexp: 
    var
    | bracketExp
    | funcCall
;

// get_func()(1,2,3)()
// (objects[69]):new(1,2,3)
//
funcCall: 
    var (funcCall_tail)+
    | bracketExp (funcCall_tail)+
;

funcCall_tail: 
    args
    | ':' name args
;



// x
// (obj1 + obj2).field
// (func())["result"]
var: 
    name (var_tail)*
    | bracketExp (var_tail)+
;

bracketExp: '(' exp ')';

var_tail:
    '[' exp ']'
    | '.' name
;

name: ID;


nameattr: name ATTRIB? ;

ATTRIB: TR_OPEN ATTRIBUTES_DEFINED TR_CLOSE ;

paramlist: namelist (',' vararg)? | vararg ;
vararg: '...' name? ;
namelist: name (',' name)* ;

args: '(' explist? ')' | STRING ;

fieldlist: field (field_sep field)* field_sep? ;
field_sep: ',' | ';' ;
field:
    exp
    | name EQ_SIGN exp
    | '[' exp ']' EQ_SIGN exp
;

unop: NOT | HASH_SIGN | MINUS_SIGN | TILDA_SIGN ; 
mulop: ASTRIX_SIGN | SLASH_SIGN | DOUBLESLASH_SIGN | PERCENT_SIGN ;
plusop: PLUS_SIGN | MINUS_SIGN ;
shiftop: SHLEFT_SIGN | SHRIGHT_SIGN ;
compop: TR_OPEN | LEQ_SIGN | TR_CLOSE | GEQ_SIGN | DOUBLEEQ_SIGN | NEQ_SIGN ; 

PLUS_SIGN: '+';
MINUS_SIGN: '-' ;
TILDA_SIGN: '~' ;
HASH_SIGN: '#';
ASTRIX_SIGN: '*';
SLASH_SIGN: '/';
DOUBLESLASH_SIGN: '//';
PERCENT_SIGN: '%';
SHLEFT_SIGN: '<<';
SHRIGHT_SIGN: '>>';

LEQ_SIGN: '<=';
GEQ_SIGN: '>=';
DOUBLEEQ_SIGN: '==';
NEQ_SIGN: '~=';

EQ_SIGN: '=';

TR_OPEN: '<';
TR_CLOSE: '>';



ATTRIBUTES_DEFINED: CONST;


// keywords
DO: 'do';        
END: 'end' ;

AND: 'and' ;      
OR: 'or';
NOT: 'not';       

IF: 'if';
ELSEIF: 'elseif';     
ELSE: 'else';      
THEN: 'then';      

WHILE: 'while';
REPEAT: 'repeat';    
UNTIL: 'until';     
FOR: 'for';       
IN: 'in';        
BREAK: 'break';    

GOTO: 'goto';

RETURN: 'return';    

FUNCTION: 'function';  

NIL: 'nil';       
TRUE: 'true';      
FALSE: 'false';     

LOCAL: 'local';     
GLOBAL: 'global' ;

CONST: 'const' ;

NUMBER: (MINUS_SIGN)? [0-9]+ ('.' [0-9]+)? ;
STRING
    : '"' ( '\\' . | ~["\\\r\n] )* '"'
    ;
// UNTERMINATED_STRING: '"' ( '\\"' | ~["\r\n] )* 
//     { 
//         throw new RuntimeException("Syntax error: unterminated string at line " + getLine());
//     }
// ;

ID: [a-zA-Z_][a-zA-Z0-9_]* ;
WS: [ \t\r\n]+ -> skip;
LINE_COMMENT: '--' ~[\r\n]* -> skip;
BLOCK_COMMENT: '--[[' .*? ']]' -> skip;