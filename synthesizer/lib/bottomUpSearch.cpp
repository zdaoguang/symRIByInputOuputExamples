#include "bottomUpSearch.hpp"
/******************************************
    Constructor
 */

// Support definition
#define GET_NUM_OF_OPS(prog, op)            ( prog->getNumberOfOpsInProg(op) )
#define GET_NUM_OF_SYMS(prog, sym)          ( prog->getNumberOfVarsInProg(sym) )
#define GET_LENGTH(prog)                    ( GET_NUM_OF_SYMS(prog, "ALL") + GET_NUM_OF_OPS(prog, "ALL") )
#define GET_EXP(prog, sym)                  ( prog->getExponentOfVarInProg(sym))

#define CHECK_NO_SYM(prog, sym)             ( GET_NUM_OF_SYMS(prog, sym) == 0 )
#define HAS_SYM(prog, sym)                  ( GET_NUM_OF_SYMS(prog, sym) > 0 )
#define CHECK_LESS_SYM(pi, pj, sym)         ( GET_NUM_OF_SYMS(pi, sym) < GET_NUM_OF_SYMS(pj, sym) )
#define CHECK_EQ_SYM(pi, pj, sym)           ( GET_NUM_OF_SYMS(pi, sym) == GET_NUM_OF_SYMS(pj, sym) )

#define DEPTH_SHOTER(pi, pj)                ( pi->depth() < pj->depth() )
#define LENGTH_SHOTER(pi, pj)               ( GET_LENGTH(pi) < GET_LENGTH(pj) )

// NO SYM
#define NO_SYM_1(prog)                      ( CHECK_NO_SYM(pi, "ALL") )
#define NO_SYM_2(pi, pj)                    ( CHECK_NO_SYM(pi, "ALL") && CHECK_NO_SYM(pj, "ALL") )

#define GET_LENGTH_SHOTER(pi, pj)           ( LENGTH_SHOTER(pi, pj) ? pi : pj )

// B SYM ONLY
#define B_SYM_ONLY_1(prog)                  ( !NO_SYM_1(prog) && CHECK_NO_SYM(prog, "I") )
#define B_SYM_ONLY_2(pi, pj)                ( !NO_SYM_2(pi, pj) && CHECK_NO_SYM(pi, "I") && CHECK_NO_SYM(pj, "I") )

// I SYM ONLY
#define I_SYM_ONLY_1(prog)                  ( !NO_SYM_1(prog) && CHECK_NO_SYM(prog, "B") )
#define I_SYM_ONLY_2(pi, pj)                ( !NO_SYM_2(pi, pj) && CHECK_NO_SYM(pi, "B") && CHECK_NO_SYM(pj, "B") )

// BOTH SYM
#define BOTH_SYM_1(prog)                    ( !CHECK_NO_SYM(prog, "B") && !CHECK_NO_SYM(prog, "I") )
#define BOTH_SYM_2(pi, pj)                  ( BOTH_SYM_1(pi) && BOTH_SYM_1(pj) )

// LENGTH RULE
#define GET_LENGTH_SHOTER(pi, pj)           ( LENGTH_SHOTER(pi, pj) ? pi : pj )
#define GET_LESS_SYM(pi, pj, sym)           ( CHECK_EQ_SYM(pi, pj, sym) ? pi : pj )

bottomUpSearch::bottomUpSearch(int depthBound,
                               vector<string> intOps,
                               vector<string> boolOps,
                               vector<string> vars,
                               vector<string> constants,
                               bool isPred,
                               vector<string> rulesToApply,
                               inputOutputs_t inputOutputs) {
    _depthBound = depthBound;
    _intOps = intOps;
    _boolOps = boolOps;
    _constants = constants;
    _inputOutputs = inputOutputs;
    _isPred = isPred;
    
    if (isPred) {
        _vars = vars;
    } else {
        for (auto v : vars) {
            if (v.find("b") != string::npos) {
                _vars.push_back(v);
            }
        }
    }
    
    int var_order = 1;
    for(auto varStr : _vars) {
        Var* var = new Var(varStr);
        BaseType* baseVar = dynamic_cast<BaseType*>(var);
        if(baseVar == nullptr) throw runtime_error("Init Var list error");
        _pList.push_back(baseVar);
        _vars_orders[varStr] = var_order;
        var_order++;
    }
    _num_of_vars = _vars.size();
    
    for (auto numStr : constants) {
        Num* num = new Num(stoi(numStr));
        BaseType* baseNum = dynamic_cast<BaseType*>(num);
        if (baseNum == nullptr) throw runtime_error("Init Num list error");
        baseNum->set_generation(10);
        _pList.push_back(baseNum);
    }
    
    for (auto prog : _pList) {
        if (prog == nullptr) throw runtime_error("Nullptr in program list");
        if (dynamic_cast<Num*>(prog)) {
            prog->set_generation(10);
        } else {
            prog->set_generation(1);
        }
    }
    
    for (auto ioe : inputOutputs) {
        if (ioe.find("_out") == ioe.end()) throw runtime_error("No _out entry in IOE");
        _max_output = max(_max_output, ioe["_out"]);
    }
}

/******************************************
    Dump Program list
*/
inline string bottomUpSearch::dumpProgram(BaseType* p) {
    if (auto var = dynamic_cast<Var*>(p)) {
        return var->toString();
    }
    else if (auto num = dynamic_cast<Num*>(p)) {
        return num->toString();
    }
    else if (auto f = dynamic_cast<F*>(p)) {
        return f->toString();
    }
    else if (auto plus = dynamic_cast<Plus*>(p)) {
        return plus->toString();
    }
    else if (auto minus = dynamic_cast<Minus*>(p)) {
        return minus->toString();
    }
    else if (auto times = dynamic_cast<Times*>(p)) {
        return times->toString();
    }
    else if (auto leftshift = dynamic_cast<Leftshift*>(p)) {
        return leftshift->toString();
    }
    else if (auto rightshift = dynamic_cast<Rightshift*>(p)) {
        return rightshift->toString();
    }
    else if (auto lt = dynamic_cast<Lt*>(p)) {
        return lt->toString();
    }
    else if (auto a = dynamic_cast<And*>(p)) {
        return a->toString();
    }
    else if (auto n = dynamic_cast<Not*>(p)) {
        return n->toString();
    }
    else if (auto ite = dynamic_cast<Ite*>(p)) {
        return ite->toString();
    }
    else {
        throw runtime_error("bottomUpSearch::dumpProgram() operates on UNKNOWN type!");
    }
    return "NoExpr";
}

void bottomUpSearch::dumpPlist(vector<BaseType*> pList) {
    cout << "[";
    for (auto prog : pList) {
        cout << dumpProgram(prog);
        if (prog != pList.back()) cout << ", ";
    }
    cout << "]" << endl;
    return;
}

void bottomUpSearch::dumpPlist() {
    cout << "[";
    for (auto prog : _pList) {
        cout << dumpProgram(prog);
        if (prog != _pList.back()) cout << ", ";
    }
    cout << "]" << endl;
    return;
}

void bottomUpSearch::dumpLangDef() {
    cout << "Dump language used:" << endl;
    cout << "    intOps: ";
    for (auto op : _intOps) cout << op << " ";
    cout << endl;
    cout << "    boolOps: ";
    for (auto op : _boolOps) cout << op << " ";
    cout << endl;
    cout << "    constants: ";
    for (auto c : _constants) cout << c << " ";
    cout << endl;
    cout << "    vars: ";
    for (auto v : _vars) cout << v << " ";
    cout << endl;
}

/******************************************
    Specify and check  growing rules
*/

inline bool bottomUpSearch::depth_rule(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    if (operand_a && operand_a->depth() >= _depthBound) return false;
    if (operand_b && operand_b->depth() >= _depthBound) return false;
    if (operand_c && operand_c->depth() >= _depthBound) return false;
    return true;
}

inline bool bottomUpSearch::generation_rule(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    int cur_generation = 0;
    if (operand_a != nullptr) cur_generation = max(cur_generation, operand_a->get_generation());
    if (operand_b != nullptr) cur_generation = max(cur_generation, operand_b->get_generation());
    if (operand_c != nullptr) cur_generation = max(cur_generation, operand_c->get_generation());
    
    if (cur_generation + 1 != prog_generation) return false;
    return true;
}

inline bool bottomUpSearch::type_rule(BaseType *operand_a, BaseType *operand_b, BaseType *operand_c, string op, int prog_generation) {
    if (op == "PLUS" || op == "MINUS" || op == "TIMES" || op == "LT" || op == "LEFTSHIFT" || op == "RIGHTSHIFT") {
        if (!(dynamic_cast<IntType*>(operand_a) && dynamic_cast<IntType*>(operand_b))) {
            return false;
        }
    }
    else if (op == "NOT") {
        if (!dynamic_cast<BoolType*>(operand_a)) {
            return false;
        }
    }
    else if (op == "AND") {
        if (!(dynamic_cast<BoolType*>(operand_a) && dynamic_cast<BoolType*>(operand_b))) {
            return false;
        }
    }
    else if (op == "ITE") {
        if (!(dynamic_cast<BoolType*>(operand_a) && dynamic_cast<IntType*>(operand_b) && dynamic_cast<IntType*>(operand_c))) {
            return false;
        }
    }
    else {
        throw runtime_error("bottomUpSearch::isGrowRuleSatisfied() operates on UNKNOWN type: " + op);
    }
    return true;
}

inline bool bottomUpSearch::elimination_free_rule(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    
    // grow NUM only by TIMES
    auto a_num = dynamic_cast<Num*>(operand_a);
    auto b_num = dynamic_cast<Num*>(operand_b);
    if (a_num != nullptr && b_num != nullptr) {
        if (op != "TIMES") return false;
        int a_value = a_num->interpret();
        int b_value = b_num->interpret();
        int c_value = a_value * b_value;
        if (c_value < 2 || a_value < 2 || b_value < 2) return false;
        else return true;
    }
    // no 0/1 TIMES op_b
    if (a_num != nullptr && b_num == nullptr) {
        int a_value = a_num->interpret();
        if (a_value < 2 && op == "TIMES") return false;
        if (a_value == 0 && op == "PLUS") return false;
    }
    // no NUM on the right handside of TIMES and PLUS
    if (b_num != nullptr && (op == "TIMES" || op == "PLUS")) {
        return false;
    }
    
    // form rule (No redunant expression rules)
    
    if (op == "PLUS" || op == "MINUS" || op == "TIMES" || op == "LEFTSHIFT" || op == "RIGHTSHIFT") {
        if (operand_a == nullptr || operand_b == nullptr) return false;
        
        bool is_a_num = ( dynamic_cast<Num*>(operand_a) != nullptr );
        bool is_a_var = ( dynamic_cast<Var*>(operand_a) != nullptr );
        bool is_a_plus = ( dynamic_cast<Plus*>(operand_a) != nullptr );
        bool is_a_times = ( dynamic_cast<Times*>(operand_a) != nullptr );
        
        bool is_b_num = ( dynamic_cast<Num*>(operand_b) != nullptr );
        bool is_b_var = ( dynamic_cast<Var*>(operand_b) != nullptr );
        bool is_b_times = ( dynamic_cast<Times*>(operand_b) != nullptr );
        bool is_b_plus = ( dynamic_cast<Plus*>(operand_b) != nullptr );
        
        int num_in_a = operand_a->getNumberOfVarsInProg("NUM");
        int num_in_b = operand_b->getNumberOfVarsInProg("NUM");
        int plus_in_a = operand_a->getNumberOfOpsInProg("PLUS");
        int plus_in_b = operand_b->getNumberOfOpsInProg("PLUS");
        
        if (op == "TIMES") {
            if ( !(is_a_var || is_a_num) ) return false;
            if ( !(is_b_var || is_b_times) ) return false;
            if ( is_a_var && _vars_orders.find(operand_a->toString()) == _vars_orders.end() ) throw runtime_error("Var a not in _vars_orders list");
            if ( is_b_var && _vars_orders.find(operand_b->toString()) == _vars_orders.end() ) throw runtime_error("Var b not in _vars_orders list");
            if ( is_a_var && is_b_var && _vars_orders[operand_a->toString()] > _vars_orders[operand_b->toString()] ) return false;
            if ( auto b_times = dynamic_cast<Times*>(operand_b) ) {
                if (dynamic_cast<Num*>(b_times->getLeft())) return false;
                if (is_a_var) {
                    if (_vars_orders[operand_a->toString()] > _vars_orders[b_times->getLeft()->toString()]) return false;
                }
            }
        }
        
        if (op == "PLUS") {
            if ( !(is_a_num || is_a_var || is_a_times) ) return false;
            if ( !(is_b_var || is_b_times || is_b_plus) ) return false;
            
            if ( (is_a_var || is_a_times) && (is_b_var || is_b_times)) {
                //cout << operand_a << " " << operand_a->toString() << " ";
                vector<int> a_lex = operand_a->getLexicalOrder(_num_of_vars, _vars_orders);
                //cout << operand_b << " " << operand_b->toString();
                //cout << endl;
                vector<int> b_lex = operand_b->getLexicalOrder(_num_of_vars, _vars_orders);
                
                if (a_lex == b_lex ||
                    !lexicographical_compare(a_lex.begin(), a_lex.end(),
                                            b_lex.begin(), b_lex.end()))
                    return false;
            }
            if ( is_b_plus ) {
                auto b_plus = dynamic_cast<Plus*>(operand_b);
                vector<int> a_lex = operand_a->getLexicalOrder(_num_of_vars, _vars_orders);
                vector<int> b_plus_left_lex = operand_b->getLexicalOrder(_num_of_vars, _vars_orders);
                if(a_lex == b_plus_left_lex ||
                   !lexicographical_compare(a_lex.begin(), a_lex.end(),
                                           b_plus_left_lex.begin(), b_plus_left_lex.end()))
                    return false;
            }
        }
    }
    
    if (op == "LT") {
        bool is_a_num = dynamic_cast<Num*>(operand_a) != nullptr;
        bool is_a_var = dynamic_cast<Var*>(operand_a) != nullptr;
        bool is_a_plus = dynamic_cast<Plus*>(operand_a) != nullptr;
        bool is_a_times = dynamic_cast<Times*>(operand_a) != nullptr;
        
        bool is_b_num = dynamic_cast<Num*>(operand_b) != nullptr;
        bool is_b_var = dynamic_cast<Var*>(operand_b) != nullptr;
        bool is_b_times = dynamic_cast<Times*>(operand_b) != nullptr;
        bool is_b_plus = dynamic_cast<Plus*>(operand_b) != nullptr;
        
        // remove expr "Num < Num"
        if (is_a_num && is_b_num) return false;
        // remove expr "Num < Num + *" and "Num + * < Num"
        if (is_a_num && is_b_plus) {
            auto b_plus = dynamic_cast<Plus*>(operand_b);
            if (dynamic_cast<Num*>(b_plus->getLeft())) return false;
        }
        if (is_b_num && is_a_plus) {
            auto a_plus = dynamic_cast<Plus*>(operand_a);
            if (dynamic_cast<Num*>(a_plus->getLeft())) return false;
        }
        // remove expr "Num < Num x *" if gcd is not 1
        if (is_a_num && is_b_times) {
            auto b_times = dynamic_cast<Times*>(operand_b);
            if (auto b_coefficient = dynamic_cast<Num*>(b_times->getLeft())) {
                if (gcd(stoi(operand_a->toString()), stoi(b_coefficient->toString())) != 1)
                    return false;
            }
        }
        if (is_b_num && is_a_times) {
            auto a_times = dynamic_cast<Times*>(operand_a);
            if (auto a_coefficient = dynamic_cast<Num*>(a_times->getLeft())) {
                if (gcd(stoi(a_coefficient->toString()), stoi(operand_b->toString())) != 1) {
                    return false;
                }
            }
        }
        
        // remove "var < var" when var == var
        if (is_a_var && is_b_var && operand_a->toString() == operand_b->toString()) return false;
        // remove "var < x ", " x < var", when var is a factor
        if (is_a_var && is_b_times) {
            auto b_times = dynamic_cast<Times*>(operand_b);
            vector<string> b_times_factors = b_times->getFactors();
            if (find(b_times_factors.begin(), b_times_factors.end(), operand_a->toString()) != b_times_factors.end())
                return false;
        }
        if (is_a_times && is_b_var) {
            auto a_times = dynamic_cast<Times*>(operand_a);
            vector<string> a_times_factors = a_times->getFactors();
            if (find(a_times_factors.begin(), a_times_factors.end(), operand_b->toString()) != a_times_factors.end()) {
                return false;
            }
        }
        // remove "var < + ", "+ < var", when var is a term
        if (is_a_var && is_b_plus) {
            auto b_plus = dynamic_cast<Plus*>(operand_b);
            vector<string> b_plus_terms = b_plus->getTerms();
            if (find(b_plus_terms.begin(), b_plus_terms.end(), operand_a->toString()) != b_plus_terms.end()) {
                return false;
            }
        }
        if (is_a_plus && is_b_var) {
            auto a_plus = dynamic_cast<Plus*>(operand_a);
            vector<string> a_plus_terms = a_plus->getTerms();
            if (find(a_plus_terms.begin(), a_plus_terms.end(), operand_b->toString()) != a_plus_terms.end()) {
                return false;
            }
        }
        
        // remove "times < times ", where share same factor
        if (is_a_times && is_b_times) {
            auto a_times = dynamic_cast<Times*>(operand_a);
            auto b_times = dynamic_cast<Times*>(operand_b);
            vector<string> a_times_factors = a_times->getFactors();
            vector<string> b_times_factors = b_times->getFactors();
            for (auto factor : a_times_factors) {
                if (find(b_times_factors.begin(), b_times_factors.end(), factor) != b_times_factors.end()) {
                    return false;
                }
            }
        }
        // remove " time < + ", "+ < time", where time is a term of +
        if (is_a_times && is_b_plus) {
            auto a_times = dynamic_cast<Times*>(operand_a);
            auto b_plus = dynamic_cast<Plus*>(operand_b);
            vector<string> b_plus_terms = b_plus->getTerms();
            string a_times_str_no_num;
            if (dynamic_cast<Num*>(a_times->getLeft())) {
                a_times_str_no_num = a_times->getRight()->toString();
            } else {
                a_times_str_no_num = a_times->toString();
            }
            if (find(b_plus_terms.begin(), b_plus_terms.end(), a_times_str_no_num) != b_plus_terms.end()) {
                return false;
            }
        }
        if (is_a_plus && is_b_times) {
            auto a_plus = dynamic_cast<Plus*>(operand_a);
            auto b_times = dynamic_cast<Times*>(operand_b);
            vector<string> a_plus_terms = a_plus->getTerms();
            string b_times_str_no_num;
            if (dynamic_cast<Num*>(b_times->getLeft())) {
                b_times_str_no_num = b_times->getRight()->toString();
            } else {
                b_times_str_no_num = b_times->toString();
            }
            if (find(a_plus_terms.begin(), a_plus_terms.end(), b_times_str_no_num) != a_plus_terms.end()) {
                return false;
            }
        }
        
        // remove " + < + " with common terms
        if (is_a_plus && is_b_plus) {
            auto a_plus = dynamic_cast<Plus*>(operand_a);
            auto b_plus = dynamic_cast<Plus*>(operand_b);
            if (dynamic_cast<Num*>(a_plus->getLeft()) && dynamic_cast<Num*>(b_plus->getLeft()))
                return false;
            vector<string> a_plus_terms = a_plus->getTerms();
            vector<string> b_plus_terms = b_plus->getTerms();
            for (auto term : a_plus_terms) {
                if (find(b_plus_terms.begin(), b_plus_terms.end(), term) != b_plus_terms.end()) {
                    return false;
                }
            }
        }
    }
    
    if (op == "AND") {
        // no F in AND
        if (dynamic_cast<F*>(operand_a) || dynamic_cast<F*>(operand_b)) return false;
        // Operand_a has to be LT
        if (!dynamic_cast<Lt*>(operand_a)) return false;
        // Operand_b has to be And/Not/Lt
        if (!(dynamic_cast<And*>(operand_b) ||
              dynamic_cast<Not*>(operand_b) ||
              dynamic_cast<Lt*>(operand_b))) return false;
        // no redundant expr
        vector<int> left_lex = operand_a->getLexicalOrder(_num_of_vars, _vars_orders);
        vector<int> right_lex;
        if (auto b_and = dynamic_cast<And*>(operand_b)) {
            right_lex = b_and->getRight()->getLexicalOrder(_num_of_vars, _vars_orders);
            if(left_lex == right_lex ||
               !lexicographical_compare(left_lex.begin(), left_lex.end(),
                                        right_lex.begin(), right_lex.end()))
                return false;
            
        } else {
            right_lex = operand_b->getLexicalOrder(_num_of_vars, _vars_orders);
            if(left_lex == right_lex ||
               !lexicographical_compare(left_lex.begin(), left_lex.end(),
                                        right_lex.begin(), right_lex.end()))
                return false;
        }
    }
    
    if (op == "NOT") {
        if (!dynamic_cast<And*>(operand_a)) return false;
    }
    
    return true;
}

inline bool bottomUpSearch::form_bias_rule(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    /*
    auto a_num = dynamic_cast<Num*>(operand_a);
    auto b_num = dynamic_cast<Num*>(operand_b);
    if (a_num && b_num) {
        int c;
        if (op == "TIMES") c = a_num->interpret() * b_num->interpret();
        if (op == "PLUS") c = a_num->interpret() + b_num->interpret();
        if (c > 100) return false;
    }
    */
    // form rule, prefered forms
    if (op == "LT") {
        // enforce "no b < *"
        if (operand_a && operand_b &&
            operand_a->getNumberOfVarsInProg("b") != 0 && operand_b->getNumberOfVarsInProg("b") != 0) return false;
        if (operand_a && operand_b &&
            operand_a->getNumberOfVarsInProg("b") == 0 && operand_b->getNumberOfVarsInProg("b") == 0) return false;
        // enforce "one isrc < *"
        if (operand_a && operand_b &&
            operand_a->getNumberOfVarsInProg("isrc") != 0 && operand_b->getNumberOfVarsInProg("isrc") != 0) return false;
        if (operand_a && operand_b &&
            operand_a->getNumberOfVarsInProg("isrc") == 0 && operand_b->getNumberOfVarsInProg("isrc") == 0) return false;
        // enforce "* < no isrc"
        //if (operand_b->getNumberOfVarsInProg("isrc") != 0) return false;
    }
    if (op == "TIMES") {
        // enforce a * Var, a <= 20
        /*
        if (auto a_num = dynamic_cast<Num*>(operand_a)) {
            if (a_num->interpret() > 20) return false;
        }
         */
        if (operand_a && operand_b &&
            operand_a->getNumberOfVarsInProg("VAR") + operand_b->getNumberOfVarsInProg("VAR") > 4)
            return false;
    }
    return true;
}

bool bottomUpSearch::isGrowRuleSatisfied(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    if (depth_rule(operand_a, operand_b, operand_c, op, prog_generation) == false) return false;
    if (type_rule(operand_a, operand_b, operand_c, op, prog_generation) == false) return false;
    if (generation_rule(operand_a, operand_b, operand_c, op, prog_generation) == false) return false;
    if (elimination_free_rule(operand_a, operand_b, operand_c, op, prog_generation) == false) return false;
    if (form_bias_rule(operand_a, operand_b, operand_c, op, prog_generation) == false) return false;
    return true;
}

/******************************************
    Grow program list
*/
inline BaseType* bottomUpSearch::growOneExpr(BaseType* operand_a, BaseType* operand_b, BaseType* operand_c, string op, int prog_generation) {
    // check rules
    if (op == "F" || !isGrowRuleSatisfied(operand_a, operand_b, operand_c, op, prog_generation)) {
        return nullptr;
    }
    
    // constant expression, only grow constant expression by times
    if (auto a = dynamic_cast<Num*>(operand_a)) {
        if (auto b = dynamic_cast<Num*>(operand_b)) {
            return dynamic_cast<BaseType*>(new Num(a, b, "TIMES"));
        }
    }
    
    if (op == "PLUS") {
        Plus* plus = new Plus(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(plus);
    }
    else if (op == "MINUS") {
        Minus* minus = new Minus(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(minus);
    }
    else if (op == "LEFTSHIFT") {
        Leftshift* leftshift = new Leftshift(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(leftshift);
    }
    else if (op == "RIGHTSHIFT") {
        Rightshift* rightshift = new Rightshift(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(rightshift);
    }
    else if (op == "TIMES") {
        Times* times = new Times(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(times);
    }
    else if (op == "ITE") {
        Ite* ite = new Ite(dynamic_cast<BoolType*>(operand_a), dynamic_cast<IntType*>(operand_b), dynamic_cast<IntType*>(operand_c));
        return dynamic_cast<BaseType*>(ite);
    }
    else if (op == "F") {
        F* f = new F();
        return dynamic_cast<BaseType*>(f);
    }
    else if (op == "NOT") {
        Not* n = new Not(dynamic_cast<BoolType*>(operand_a));
        return dynamic_cast<BaseType*>(n);
    }
    else if (op == "AND") {
        And* a = new And(dynamic_cast<BoolType*>(operand_a), dynamic_cast<BoolType*>(operand_b));
        return dynamic_cast<BaseType*>(a);
    }
    else if (op == "LT") {
        Lt* lt = new Lt(dynamic_cast<IntType*>(operand_a), dynamic_cast<IntType*>(operand_b));
        return dynamic_cast<BaseType*>(lt);
    }
    else {
        throw runtime_error("bottomUpSearch::growOneExpr() operates on UNKNOWN type!");
    }
    
    return nullptr;
}

void bottomUpSearch::grow(int prog_generation) {
    
    int program_list_length = _pList.size();
    for (auto op : _intOps) {
        
        if (op == "PLUS" || op == "TIMES" || op == "MINUS" || op == "LEFTSHIFT" || op == "RIGHTSHIFT") {
            int cnt = 0;
            for (int i = 0; i < program_list_length; i++) {
                for (int j = 0; j < program_list_length; j++) {
                    BaseType* new_expr = growOneExpr(_pList[i], _pList[j], nullptr, op, prog_generation);
                    if (new_expr != nullptr) _pList.push_back(new_expr);
                }
            }
        }
        else if (op == "ITE") {
            for (int i = 0; i < program_list_length; i++) {
                for (int j = 0; j < program_list_length; j++) {
                    for (int k = 0; k < program_list_length; k++) {
                        BaseType* new_expr = growOneExpr(_pList[i], _pList[j], _pList[k], op, prog_generation);
                        if (new_expr != nullptr)
                            _pList.push_back(new_expr);
                    }
                }
            }
        }
    }
    
    for (auto op : _boolOps) {
        
        if (op == "F") {
            BaseType* new_expr = growOneExpr(nullptr, nullptr, nullptr, op, prog_generation);
            if (new_expr != nullptr)
                _pList.push_back(new_expr);
        }
        else if (op == "NOT") {
            for (int j = 0; j < program_list_length; j++) {
                BaseType* new_expr = growOneExpr(_pList[j], nullptr, nullptr, op, prog_generation);
                if (new_expr != nullptr)
                    _pList.push_back(new_expr);
            }
        }
        else if (op == "AND") {
            for (int j = 0; j < program_list_length; j++) {
                for (int k = 0; k < program_list_length; k++) {
                    BaseType* new_expr = growOneExpr(_pList[j], _pList[k], nullptr, op, prog_generation);
                    if (new_expr != nullptr)
                        _pList.push_back(new_expr);
                }
            }
        }
        else if (op == "LT") {
            for (int j = 0; j < program_list_length; j++) {
                for (int k = 0; k < program_list_length; k++) {
                    BaseType* new_expr = growOneExpr(_pList[j], _pList[k], nullptr, op, prog_generation);
                    if (new_expr != nullptr)
                        _pList.push_back(new_expr);
                }
            }
        }
    }
    
    for (int i = program_list_length; i < _pList.size(); i++) {
        if (_pList[i] && !dynamic_cast<Num*>(_pList[i]))
            _pList[i]->set_generation(prog_generation);
    }
    
    return;
}

/******************************************
    Eliminate equvalent programs
*/
inline int bottomUpSearch::evaluateIntProgram(BaseType* p, int inputOutputId) {
    
    if (_intResultRecord.find(make_pair(p, inputOutputId)) != _intResultRecord.end()) {
        return _intResultRecord[make_pair(p, inputOutputId)];
    }
    
    int pValue;
    if (auto num = dynamic_cast<Num*>(p)) {
        pValue = num->interpret();
    }
    else if (auto var = dynamic_cast<Var*>(p)) {
        pValue = var->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto plus = dynamic_cast<Plus*>(p)) {
        pValue = plus->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto times = dynamic_cast<Times*>(p)) {
        pValue = times->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto minus = dynamic_cast<Minus*>(p)) {
        pValue = minus->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto leftshift = dynamic_cast<Leftshift*>(p)) {
        pValue = leftshift->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto rightshift = dynamic_cast<Rightshift*>(p)) {
        pValue = rightshift->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto ite = dynamic_cast<Ite*>(p)) {
        pValue = ite->interpret(_inputOutputs[inputOutputId]);
    }
    else {
        throw runtime_error("bottomUpSearch::evaluateIntProgram() operates on UNKNOWN type!");
    }
    
    _intResultRecord[make_pair(p, inputOutputId)] = pValue;
    return pValue;
}

inline bool bottomUpSearch::evaluateBoolProgram(BaseType* p, int inputOutputId) {
    if (_boolResultRecord.find(make_pair(p, inputOutputId)) != _boolResultRecord.end()) {
        return _boolResultRecord[make_pair(p, inputOutputId)];
    }
    
    bool pValue;
    if (auto f = dynamic_cast<F*>(p)) {
        pValue = f->interpret();
    }
    else if (auto n = dynamic_cast<Not*>(p)) {
        pValue = n->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto a = dynamic_cast<And*>(p)) {
        pValue = a->interpret(_inputOutputs[inputOutputId]);
    }
    else if (auto lt = dynamic_cast<Lt*>(p)) {
        pValue = lt->interpret(_inputOutputs[inputOutputId]);
    }
    else {
        throw runtime_error("bottomUpSearch::evaluateBoolProgram() operates on UNKNOWN type!");
    }
    
    _boolResultRecord[make_pair(p, inputOutputId)] = pValue;
    return pValue;
}

inline bool bottomUpSearch::checkTwoProgramsEqual(BaseType* pi, BaseType* pj) {
    
    if (dynamic_cast<IntType*>(pi) && dynamic_cast<IntType*>(pj)) {
        for (int i = 0; i < _inputOutputs.size(); i++) {
            if (evaluateIntProgram(pi, i) != evaluateIntProgram(pj, i)) {
                return false;
            }
        }
    }
    else if (dynamic_cast<BoolType*>(pi) && dynamic_cast<BoolType*>(pj)) {
        for (int i = 0; i < _inputOutputs.size(); i++) {
            if (evaluateBoolProgram(pi, i) != evaluateBoolProgram(pj, i)) {
                return false;
            }
        }
    } else {
        return false;
    }
    return true;
}

inline BaseType* bottomUpSearch::elimOneProgWithRules(BaseType* pi, BaseType* pj) {
    
    if (pi == nullptr) {
        return pj;
    }
    if (pj == nullptr) {
        return pi;
    }
    
    BaseType* progToKeep = pi;
        
    // Apply common rules
    if (NO_SYM_2(pi, pj) ) {
        progToKeep = GET_LENGTH_SHOTER(pi, pj);
    }
    
    // Apply different rules for predicts and terms
    if (_isPred) {
        if (HAS_SYM(pi, "B") > 0 && HAS_SYM(pj, "B") > 0) {
            if (HAS_SYM(pi, "Isrc") > 0 && HAS_SYM(pj, "Isrc") > 0) {
                if (HAS_SYM(pi, "Isnk") > 0 && HAS_SYM(pj, "Isnk") > 0) {
                    progToKeep = GET_LESS_SYM(pi, pj, "ALL");
                } else {
                    if (HAS_SYM(pi, "Isnk") > 0) {
                        progToKeep = pj;
                    } else if (HAS_SYM(pj, "Isnk") > 0) {
                        progToKeep = pi;
                    } else {
                        if (GET_EXP(pi, "Isrc") < GET_EXP(pj, "Isrc")) {
                            progToKeep = pi;
                        } else if (GET_EXP(pi, "Isrc") > GET_EXP(pj, "Isrc")) {
                            progToKeep = pj;
                        } else {
                            if (GET_EXP(pi, "B") < GET_EXP(pj, "B")) {
                                progToKeep = pi;
                            } else if (GET_EXP(pi, "B") > GET_EXP(pj, "B")) {
                                progToKeep = pj;
                            } else {
                                progToKeep = GET_LESS_SYM(pi, pj, "ALL");
                            }
                        }
                    }
                }
            } else {
                if (HAS_SYM(pi, "Isrc") > 0) {
                    progToKeep = pi;
                } else if (HAS_SYM(pj, "Isrc") > 0) {
                    progToKeep = pj;
                } else {
                    if (GET_NUM_OF_SYMS(pi, "B") < GET_NUM_OF_SYMS(pj, "B")) {
                        progToKeep = pi;
                    } else if (GET_NUM_OF_SYMS(pi, "B") > GET_NUM_OF_SYMS(pj, "B")) {
                        progToKeep = pj;
                    } else {
                        progToKeep = GET_LENGTH_SHOTER(pi, pj);
                    }
                }
            }
        } else {
            if (HAS_SYM(pi, "B") > 0) {
                progToKeep = pi;
            } else if (HAS_SYM(pj, "B") > 0) {
                progToKeep = pj;
            } else {
                progToKeep = GET_LENGTH_SHOTER(pi, pj);
            }
        }
        
        /*
        if (BOTH_SYM_2(pi, pj)) {
            progToKeep = GET_LESS_SYM(pi, pj, "ALL");
        } else {
            if (BOTH_SYM_1(pi)) {
                progToKeep = pi;
            } else if (BOTH_SYM_1(pj)) {
                progToKeep = pj;
            } else {
                progToKeep = GET_LESS_SYM(pi, pj, "ALL");
            }
        }
        */
    } else {
        
        if (HAS_SYM(pi, "B") > 0 && HAS_SYM(pj, "B") > 0) {
            if (HAS_SYM(pi, "Isrc") > 0 && HAS_SYM(pj, "Isrc") > 0) {
                if (HAS_SYM(pi, "Isnk") > 0 && HAS_SYM(pj, "Isnk") > 0) {
                    progToKeep = GET_LESS_SYM(pi, pj, "ALL");;
                } else {
                    if (HAS_SYM(pi, "Isnk") > 0) {
                        progToKeep = pi;
                    } else if (HAS_SYM(pj, "Isnk") > 0) {
                        progToKeep = pj;
                    } else {
                        progToKeep = GET_LESS_SYM(pi, pj, "ALL");
                    }
                }
            } else {
                if (HAS_SYM(pi, "Isrc") > 0) {
                    progToKeep = pj;
                } else if (HAS_SYM(pj, "Isrc") > 0) {
                    progToKeep = pi;
                } else {
                    progToKeep = GET_LESS_SYM(pi, pj, "ALL");
                }
            }
        } else {
            if (HAS_SYM(pi, "B") > 0) {
                progToKeep = pi;
            } else if (HAS_SYM(pj, "B") > 0) {
                progToKeep = pj;
            } else {
                progToKeep = GET_LENGTH_SHOTER(pi, pj);
            }
        }
        
        /*
        if (B_SYM_ONLY_2(pi, pj)) {
            return GET_LENGTH_SHOTER(pi, pj);
        } else {
            if (CHECK_LESS_SYM(pi, pj, "I")) {
                return pi;
                
            } else {
                return pj;
            }
        }
        */
    }
    
    if (progToKeep == pi) {
        for (int inputOutputId = 0; inputOutputId < _inputOutputs.size(); inputOutputId++) {
            _boolResultRecord.erase(make_pair(pj, inputOutputId));
            _intResultRecord.erase(make_pair(pj, inputOutputId));
        }
    } else {
        for (int inputOutputId = 0; inputOutputId < _inputOutputs.size(); inputOutputId++) {
            _boolResultRecord.erase(make_pair(pi, inputOutputId));
            _intResultRecord.erase(make_pair(pi, inputOutputId));
        }
    }
    
    return progToKeep;
}

void bottomUpSearch::elimEquvalents() {
    vector<BaseType*> progToKeepList;
    vector<bool> eqFlag(_pList.size() ,false);
    
    for (int i = 0; i < _pList.size(); i++) {
        if (eqFlag[i] == true) {
            continue;
        }
        BaseType* pi = _pList[i];
        
        /* reserve all variables */
        if (dynamic_cast<Var*>(pi)) {
            progToKeepList.push_back(pi);
            continue;
        }
        
        /* Find all programs that equal */
        vector<BaseType*> eqPList;
        eqPList.push_back(pi);
        for (int j = i+1; j < _pList.size(); j++) {
            BaseType* pj = _pList[j];
            
            if (checkTwoProgramsEqual(pi, pj)) {
                eqFlag[j] = true;
                eqPList.push_back(pj);
            }
        }
        
        /* Find the program to keep */
        BaseType* progToKeep = nullptr;
        for (auto prog : eqPList) {
            //if (eqPList.size() > 1) cout << prog->toString() << " ";
            progToKeep = elimOneProgWithRules(progToKeep, prog);
        }
        //if (eqPList.size() > 1) cout << endl;
        
        for (int inputOutputId = 0; inputOutputId < _inputOutputs.size(); inputOutputId++) {
            _boolResultRecord.erase(make_pair(pi, inputOutputId));
            _intResultRecord.erase(make_pair(pi, inputOutputId));
        }
        
        /* random choose program to keep */
        //srand((unsigned)time(nullptr));
        //programToKeep.push_back(eqPList[rand() % eqPList.size()]);
        /* always keep the first program, which is short in depth */
        progToKeepList.push_back(progToKeep);
    }
    
    _boolResultRecord.clear();
    _intResultRecord.clear();
    _pList = progToKeepList;
    return;
}

void bottomUpSearch::elimMaxEvaluatedValueOutOfBounds() {
    int number_of_programs = _pList.size();
    vector<bool> keep_flag(number_of_programs, true);
    for (int i = 0; i < number_of_programs; i++) {
        BaseType* program = _pList[i];
        if (auto int_program = dynamic_cast<IntType*>(program)) {
            for (auto ioe : _inputOutputs) {
                int program_value = int_program->interpret(ioe);
                if (program_value > ioe["_out"] && ioe["_out"] != 0) {
                    keep_flag[i] = false;
                    break;
                }
            }
        }
    }
    
    vector<BaseType*> programs_to_keep;
    for (int i = 0; i < number_of_programs; i++) {
        if (keep_flag[i]) {
            programs_to_keep.push_back(_pList[i]);
        }
    }
    _pList = programs_to_keep;
    return;
}

/******************************************
    Check correct
 */
bool bottomUpSearch::isCorrect(BaseType* p) {
    if (_isPred) {
        if (dynamic_cast<BoolType*>(p)) {
                /* all true or all false are both correct program */
                bool allFalse = true;
                bool allTrue = true;
        
                for (int i = 0; i < _inputOutputs.size(); i++) {
                    
                    if (!(_inputOutputs[i]["_out"] == 0 || _inputOutputs[i]["_out"] == 1)) {
                        return false;
                    }
                    
                    if (evaluateBoolProgram(p, i) != _inputOutputs[i]["_out"]) {
                        allTrue = false;
                    } else {
                        allFalse = false;
                    }

                    if (!allTrue && !allFalse) {
                        return false;
                    }
                }
                if (allFalse == true) {
                    BoolType* booli = dynamic_cast<BoolType*>(p);
                    p = new Not(booli);
                }
            }
        else {
            return false;
        }
    }
    else {
        if (dynamic_cast<IntType*>(p)) {
            for (int i = 0; i < _inputOutputs.size(); i++) {
                if (evaluateIntProgram(p, i) != _inputOutputs[i]["_out"]) {
                    return false;
                }
            }
        }
        else {
            return false;
        }
    }
    
    return true;
}

inline string bottomUpSearch::getCorrect(int prog_generation) {
    for (auto prog : _pList) {
        if (prog->depth() == prog_generation && isCorrect(prog)) {
#ifdef DEBUG
            cout << "SynProg: " << dumpProgram(prog) << endl;
#endif
            return dumpProgram(prog);
        }
    }
    return "";
}

string bottomUpSearch::search() {

#ifdef DEBUG
    cout << "Init pList size " << _pList.size() << ", check correct" << endl;
#endif
    
    //elimEquvalents();
    //dumpPlist();
    
    int prog_generation = 1;
    while (getCorrect(prog_generation) == "") {
#ifdef DEBUG
        cout << "Current generation " << prog_generation << endl;
        cout << "Current pList size " << _pList.size() << ", grow" << endl;
#endif
        //dumpPlist();
        prog_generation++;
        grow(prog_generation);
        //dumpPlist();
#ifdef DEBUG
        cout << "Current pList size " << _pList.size() << ", eliminate equvalents" << endl;
#endif
        if (!_isPred) elimMaxEvaluatedValueOutOfBounds();
        if (_isPred) elimEquvalents();
        
        //dumpPlist();
#ifdef DEBUG
        cout << "Current pList size " << _pList.size() << ", check correct" << endl;
#endif
    }
    
    return getCorrect(prog_generation);
}
