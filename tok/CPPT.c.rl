#include "abc/INT.h"
#include "abc/PRO.h"
#include "CPPT.h"

ok64 CPPTonComment (u8cs tok, CPPTstate* state);
ok64 CPPTonString (u8cs tok, CPPTstate* state);
ok64 CPPTonNumber (u8cs tok, CPPTstate* state);
ok64 CPPTonPreproc (u8cs tok, CPPTstate* state);
ok64 CPPTonWord (u8cs tok, CPPTstate* state);
ok64 CPPTonPunct (u8cs tok, CPPTstate* state);
ok64 CPPTonSpace (u8cs tok, CPPTstate* state);

%%{

machine CPPT;

alphtype unsigned char;

any8 = (0x00..0xff);
ws = [ \t\r\n\f\v];
idalpha = [a-zA-Z_$];
idalnum = [a-zA-Z_$0-9];
dgt = [0-9];
xdgt = [0-9a-fA-F];
odgt = [0-7];

esc = [\\] ( [abefnrtv\\'\"?0]
           | odgt{1,3}
           | [x] xdgt+
           | [u] xdgt{4}
           | [U] xdgt{8} );

strpfx = [LuU] | "u8";

action on_comment {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonComment(tok, state);
    if (o!=OK) fbreak;
}
action on_string {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonString(tok, state);
    if (o!=OK) fbreak;
}
action on_number {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) fbreak;
}
action on_preproc {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPreproc(tok, state);
    if (o!=OK) fbreak;
}
action on_word {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonWord(tok, state);
    if (o!=OK) fbreak;
}
action on_punct {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) fbreak;
}
action on_space {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonSpace(tok, state);
    if (o!=OK) fbreak;
}

# DOG-007: capture the raw-string delimiter span (between `"` and `(`),
# then in the opener action skip the opaque body to `)<delim>"`.
action rs_d0 { rsd[0] = (u8c*)fpc; }
action rs_d1 { rsd[1] = (u8c*)fpc; }
action on_rawstr {
    size_t n = $size(rsd);
    u8c* q = (u8c*)rsd[1] + 1;       // first body byte, just past the `(`
    u8c* end = NULL;
    while (q + n + 1 < pe) {
        u8cs run = {q + 1, q + 1 + n};
        if (*q == ')' && q[n+1] == '"' && $eq(rsd, run)) {
            end = q + n + 2;
            break;
        }
        ++q;
    }
    if (end == NULL) { o = CPPTBAD; fbreak; }
    tok[0] = (u8c*)ts;
    tok[1] = end;
    o = CPPTonString(tok, state);
    if (o!=OK) fbreak;
    fexec end;          // resume the scanner just past the close
    fgoto main;
}

bscont = [\\] [\r]? [\n];

ddig = dgt+ ( ['] dgt+ )*;
xdig = xdgt+ ( ['] xdgt+ )*;
bdig = [01]+ ( ['] [01]+ )*;

isuf = ( [uU] ([lL] [lL]?)? | ([lL] [lL]?) [uU]? )?;
fsuf = [fFlLdD]?;

main := |*

    # ---- comments ----
    "//" [^\n]*                                                   => on_comment;
    "/*" ( any8 - [*] | [*]+ (any8 - [*/]) )* [*]+ "/"          => on_comment;

    # ---- raw strings R"delim(...)delim" (DOG-007) ----
    # capture <delim>, the action skips the opaque body to `)<delim>"`.
    strpfx? "R" ["] %rs_d0 ( any8 - ["()\\ \t\r\n\f\v] ){0,16} ("(" >rs_d1)  => on_rawstr;

    # ---- string literals ----
    strpfx? ["] ( esc | any8 - ["\\] )* ["]                      => on_string;

    # ---- character literals ----
    strpfx? ['] ( esc | any8 - ['\\] )* [']                      => on_string;

    # ---- numbers ----
    "0" [xX] xdig ("." xdig?)? ([pP] [+\-]? ddig) fsuf          => on_number;
    "0" [xX] xdig isuf                                           => on_number;
    "0" [bB] bdig isuf                                           => on_number;
    "0" odgt+ isuf                                               => on_number;
    ddig "." ddig? ([eE] [+\-]? ddig)? fsuf                     => on_number;
    "." ddig ([eE] [+\-]? ddig)? fsuf                            => on_number;
    ddig [eE] [+\-]? ddig fsuf                                   => on_number;
    # decimal with user-defined literal suffix
    ddig ( [_] idalpha idalnum* )?                                => on_number;

    # ---- preprocessor ----
    [#] [ \t]* ("include" | "define" | "undef" |
                "ifdef" | "ifndef" | "if" | "elif" |
                "else" | "endif" |
                "pragma" | "error" | "warning" | "line")          => on_preproc;
    [#] [ \t]* idalpha idalnum*                                   => on_preproc;
    [#]                                                           => on_punct;

    # ---- identifiers ----
    idalpha idalnum*                                              => on_word;

    # ---- multi-char operators ----
    "<=>" | "::" |
    ">>=" | "<<=" | "+=" | "-=" | "*=" | "/=" | "%=" |
    "&=" | "^=" | "|=" | ">>" | "<<" | "++" | "--" |
    "->" | "&&" | "||" | "<=" | ">=" | "==" | "!=" |
    "..."                                                         => on_punct;

    # ---- single-char punctuation ----
    (any8 - idalpha - dgt - ws - ["'#] - [.])                   => on_punct;
    [.]                                                           => on_punct;

    # ---- whitespace ----
    ws+                                                           => on_space;

*|;

}%%

%%write data;

ok64 CPPTLexer(CPPTstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    int act = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u8c *ts = NULL;
    u8c *te = NULL;
    ok64 o = OK;

    u8cs rsd = {NULL, NULL};        // DOG-007: raw-string delimiter capture

    u8cs tok = {p, p};

    %% write init;
    %% write exec;

    state->data[0] = p;
    if (o==OK && cs < CPPT_first_final)
        o = CPPTBAD;

    return o;
}
