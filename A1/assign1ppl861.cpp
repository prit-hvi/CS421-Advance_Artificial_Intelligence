#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>

using namespace std;

// Term: can be a variable (V), atom (A), or function (F), we will use this struct to represent all three types of terms in our unification algorithm.
struct Term {
    char type;  
    string name;
    vector<Term*> args;
    Term(char t, string n) : type(t), name(n) {}
};

map<string, Term*> theta;

Term* parse(const string& expr, int& pos);

// Skip spaces in the input expression
void skipSpaces(const string& expr, int& pos) {
    while (pos < expr.length() && expr[pos] == ' ') pos++;
}

// We are breaking down the terms into their types (variable and function) and parsing them accordingly. 
// Variables are uppercase, and functions are lowercase. Functions can have arguments which are also terms.
Term* parseAtom(const string& expr, int& pos) {
    skipSpaces(expr, pos);
    
    if (pos >= expr.length()) return nullptr;
    
    if (isupper(expr[pos])) {
        string name;
        while (pos < expr.length() && isalnum(expr[pos])) {
            name += expr[pos];
            pos++;
        }
        return new Term('V', name);
    }
    
    if (islower(expr[pos])) {
        string name;
        while (pos < expr.length() && isalnum(expr[pos])) {
            name += expr[pos];
            pos++;
        }
        
        skipSpaces(expr, pos);
        
        if (pos < expr.length() && expr[pos] == '(') {
            pos++;  
            vector<Term*> args;
            
            while (pos < expr.length() && expr[pos] != ')') {
                skipSpaces(expr, pos);
                if (expr[pos] == ')') break;
                
                Term* arg = parse(expr, pos);
                args.push_back(arg);
                
                skipSpaces(expr, pos);
                if (pos < expr.length() && expr[pos] == ',') {
                    pos++;  
                }
            }
            
            pos++;  
            Term* func = new Term('F', name);
            func->args = args;
            return func;
        } else {
            return new Term('A', name);
        }
    }
    
    return nullptr;
}

Term* parse(const string& expr, int& pos) {
    return parseAtom(expr, pos);
}

// Check if variable occurs in term (occurs check)
bool occursCheck(const string& var, Term* term) {
    if (term == nullptr) return false;
    
    if (term->type == 'V') {
        if (theta.find(term->name) != theta.end()) {
            return occursCheck(var, theta[term->name]);
        }
        return term->name == var;
    }
    
    if (term->type == 'F') {
        for (Term* arg : term->args) {
            if (occursCheck(var, arg)) return true;
        }
    }
    
    return false;
}

// Replace all occurrences of variable X with term T
void replaceVar(const string& var, Term* replacement, Term* term) {
    if (term == nullptr) return;
    
    if (term->type == 'V') {
        if (term->name == var) {
            term->type = replacement->type;
            term->name = replacement->name;
            term->args = replacement->args;
        }
    } else if (term->type == 'F') {
        for (Term* arg : term->args) {
            replaceVar(var, replacement, arg);
        }
    }
}

// Apply substitution to a term
Term* applySubst(Term* term) {
    if (term == nullptr) return nullptr;
    
    if (term->type == 'V') {
        if (theta.find(term->name) != theta.end()) {
            return applySubst(theta[term->name]);
        }
        return term;
    }
    
    if (term->type == 'F') {
        for (auto& arg : term->args) {
            arg = applySubst(arg);
        }
    }
    
    return term;
}

// Print term
void printTerm(Term* term) {
    if (term == nullptr) return;
    
    term = applySubst(term);
    
    if (term->type == 'V' || term->type == 'A') {
        cout << term->name;
    } else if (term->type == 'F') {
        cout << term->name << "(";
        for (int i = 0; i < term->args.size(); i++) {
            printTerm(term->args[i]);
            if (i + 1 < term->args.size()) cout << ", ";
        }
        cout << ")";
    }
}

// Main unification algorithm
bool unify(Term* t1, Term* t2) {
    theta.clear();
    
    vector<pair<Term*, Term*>> stack;
    stack.push_back({t1, t2});
    
    while (!stack.empty()) {
        auto [ptr1, ptr2] = stack.back();
        stack.pop_back();
        
        ptr1 = applySubst(ptr1);
        ptr2 = applySubst(ptr2);
        
        if (ptr1->type == ptr2->type && ptr1->name == ptr2->name && 
            ptr1->args.size() == ptr2->args.size()) {
            
            if (ptr1->type != 'F') continue;
            
            for (int i = 0; i < ptr1->args.size(); i++) {
                stack.push_back({ptr1->args[i], ptr2->args[i]});
            }
        }
       
        else {
           
            if (ptr1->type == 'V') {
                if (occursCheck(ptr1->name, ptr2)) {
                    return false;  
                }
                theta[ptr1->name] = ptr2;
            } else if (ptr2->type == 'V') {
                if (occursCheck(ptr2->name, ptr1)) {
                    return false; 
                }
                theta[ptr2->name] = ptr1;
            } else {
                return false;  
            }
        }
    }
    
    return true;
}

// Print substitution
void printSubst() {
    if (theta.empty()) {
        cout << "no bindings";
        return;
    }
    
    bool first = true;
    for (auto& [var, term] : theta) {
        if (!first) cout << ", ";
        cout << var << " = ";
        printTerm(term);
        first = false;
    }
}

int main() {

    
    vector<pair<string, string>> userInput;
    cout << "Enter your terms:\n";
    
    string line1, line2;
    cout << "Term 1, press enter when done: ";
    getline(cin, line1);
    cout << "Term 2, press enter when done: ";
    getline(cin, line2);
        
    userInput.push_back({line1, line2});
    
    int uiPos1 = 0, uiPos2 = 0;
    Term* uiT1 = parse(userInput[0].first, uiPos1);
    Term* uiT2 = parse(userInput[0].second, uiPos2);
    cout << "Unifying...\n";
    cout << "Term 1: " << userInput[0].first << endl;
    cout << "Term 2: " << userInput[0].second << endl;

    if (unify(uiT1, uiT2)) {
        cout << "Result: ";
        printSubst();
        cout << "\nyes\n\n";
    } else {
        cout << "Result: no\n\n";
    }

    cout << "Re-run the code to unify another pair of terms.\n";
    
    return 0;
}
